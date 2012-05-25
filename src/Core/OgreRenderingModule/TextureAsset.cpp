// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "TextureAsset.h"
#include "OgreRenderingModule.h"

#include "Profiler.h"
#include "AssetCache.h"
#include "LoggingFunctions.h"

#include <QPixmap>
#include <QRect>
#include <QFontMetrics>
#include <QPainter>
#include <QFileInfo>

#include <Ogre.h>

#ifdef WIN32
#include <squish.h>
#endif

#if defined(DIRECTX_ENABLED) && defined(WIN32)
#ifdef SAFE_DELETE
#undef SAFE_DELETE
#endif
#ifdef SAFE_DELETE_ARRAY
#undef SAFE_DELETE_ARRAY
#endif
#include <d3d9.h>
#include <OgreD3D9RenderSystem.h>
#include <OgreD3D9HardwarePixelBuffer.h>
#endif

#include "MemoryLeakCheck.h"

TextureAsset::TextureAsset(AssetAPI *owner, const QString &type_, const QString &name_) :
    IAsset(owner, type_, name_), loadTicket_(0)
{
    ogreAssetName = AssetAPI::SanitateAssetRef(this->Name().toStdString()).c_str();
}

TextureAsset::~TextureAsset()
{
    Unload();
}

bool TextureAsset::LoadFromFile(QString filename)
{
    bool allowAsynchronous = true;
    if (assetAPI->GetFramework()->IsHeadless() || assetAPI->GetFramework()->HasCommandLineParameter("--no_async_asset_load") || !assetAPI->GetAssetCache() || (OGRE_THREAD_SUPPORT == 0))
        allowAsynchronous = false;
    QString cacheDiskSource;
    if (allowAsynchronous)
    {
        cacheDiskSource = assetAPI->GetAssetCache()->FindInCache(Name());
        if (cacheDiskSource.isEmpty())
            allowAsynchronous = false;
    }
    
    if (allowAsynchronous)
        return DeserializeFromData(0, 0, true);
    else
        return IAsset::LoadFromFile(filename);
}

bool TextureAsset::DeserializeFromData(const u8 *data, size_t numBytes, bool allowAsynchronous)
{
    if (assetAPI->GetFramework()->HasCommandLineParameter("--notextures"))
    {
        assetAPI->AssetLoadCompleted(Name());
        return true;
    }
    
    PROFILE(TextureAsset_DeserializeFromData);

    // A NullAssetFactory has been registered on headless mode.
    // We should never be here in headless mode.
    assert(!assetAPI->IsHeadless());

    if (assetAPI->GetFramework()->IsHeadless() || assetAPI->GetFramework()->HasCommandLineParameter("--no_async_asset_load") || !assetAPI->GetAssetCache() || (OGRE_THREAD_SUPPORT == 0))
        allowAsynchronous = false;
    QString cacheDiskSource;
    if (allowAsynchronous)
    {
        cacheDiskSource = assetAPI->GetAssetCache()->FindInCache(Name());
        if (cacheDiskSource.isEmpty())
            allowAsynchronous = false;
    }
    
    // Asynchronous loading
    // 1. AssetAPI allows a asynch load. This is false when called from LoadFromFile(), LoadFromCache() etc.
    // 2. We have a rendering window for Ogre as Ogre::ResourceBackgroundQueue does not work otherwise. Its not properly initialized without a rendering window.
    // 3. The Ogre we are building against has thread support.
    if (allowAsynchronous)
    {
        // We can only do threaded loading from disk, and not any disk location but only from asset cache.
        // local:// refs will return empty string here and those will fall back to the non-threaded loading.
        // Do not change this to do DiskCache() as that directory for local:// refs will not be a known resource location for ogre.
        QFileInfo fileInfo(cacheDiskSource);
        std::string sanitatedAssetRef = fileInfo.fileName().toStdString();
        loadTicket_ = Ogre::ResourceBackgroundQueue::getSingleton().load(Ogre::TextureManager::getSingleton().getResourceType(),
                          sanitatedAssetRef, OgreRenderer::OgreRenderingModule::CACHE_RESOURCE_GROUP, false, 0, 0, this);
        return true;
    }

    if (!data)
    {
        LogError("TextureAsset::DeserializeFromData failed: Cannot deserialize from input null pointer!");
        return false;
    }
    if (numBytes == 0)
    {
        LogError("TextureAsset::DeserializeFromData failed: numBytes == 0!");
        return false;
    }

    // Synchronous loading
    try
    {
        // Convert the data into Ogre's own DataStream format.
        std::vector<u8> tempData(data, data + numBytes);
#include "DisableMemoryLeakCheck.h"
        Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&tempData[0], tempData.size(), false));
#include "EnableMemoryLeakCheck.h"
        // Load up the image as an Ogre CPU image object.
        Ogre::Image image;
        image.load(stream);

        // If we are submitting a .dds file which did not contain mip maps, don't have Ogre generating them either.
        // Reasons:
        // 1. Not all textures need mipmaps, i.e. if the texture is always shown with 1:1 texel-to-pixel ratio, then the mip levels are never needed.
        // 2. Ogre has a bug on Apple, that it fails to generate mipmaps for .dds files which contain only one mip level and are DXT1-compressed (it tries to autogenerate, but always results in black texture data)
        // 3. If the texture is updated dynamically, we might not afford to regenerate mips at each update.
        int numMipmapsInImage = image.getNumMipmaps(); // Note: This is actually numMipmaps - 1: Ogre doesn't think the first level is a mipmap.
        int numMipmapsToUseOnGPU = Ogre::MIP_DEFAULT;
        if (numMipmapsInImage == 0 && this->Name().endsWith(".dds", Qt::CaseInsensitive))
            numMipmapsToUseOnGPU = 0;

        if (ogreTexture.isNull()) // If we are creating this texture for the first time, create a new Ogre::Texture object.
        {
            ogreAssetName = AssetAPI::SanitateAssetRef(this->Name().toStdString()).c_str();
            
            // Optionally load textures to default pool for memory use debugging. Do not use in production use due to possible crashes on device loss & missing mipmaps!
            // Note: this does not affect async loading path, so specify additionally --no_async_asset_load to be sure textures are loaded through this path
            // Furthermore, it may still allocate virtual memory address space due to using AGP memory mapping (we would not actually need a dynamic texture, but there's no way to tell Ogre that)
            if (assetAPI->GetFramework()->HasCommandLineParameter("--d3ddefaultpool"))
            {
                ogreTexture = Ogre::TextureManager::getSingleton().createManual(ogreAssetName.toStdString(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D,image.getWidth(), 
                    image.getHeight(), numMipmapsToUseOnGPU, image.getFormat(), Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
                ogreTexture->loadImage(image);
            }
            else
            {
                ogreTexture = Ogre::TextureManager::getSingleton().loadImage(ogreAssetName.toStdString(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, image, Ogre::TEX_TYPE_2D, 
                    numMipmapsToUseOnGPU);
            }
        }
        else // If we're loading on top of an Ogre::Texture we've created before, don't lose the old Ogre::Texture object, but reuse the old.
        {    // This will allow all existing materials to keep referring to this texture, and they'll get the updated texture image immediately.
            ogreTexture->freeInternalResources(); 

            if (image.getWidth() != ogreTexture->getWidth() || image.getHeight() != ogreTexture->getHeight() || image.getFormat() != ogreTexture->getFormat())
            {
                ogreTexture->setWidth(image.getWidth());
                ogreTexture->setHeight(image.getHeight());
                ogreTexture->setFormat(image.getFormat());
            }

            if (ogreTexture->getBuffer().isNull())
            {
                LogError("DeserializeFromData: Failed to create texture " + this->Name() + ": OgreTexture::getBuffer() was null!");
                return false;
            }

            Ogre::PixelBox pixelBox(Ogre::Box(0,0, image.getWidth(), image.getHeight()), image.getFormat(), (void*)image.getData());
            ogreTexture->getBuffer()->blitFromMemory(pixelBox);

            ogreTexture->createInternalResources();
        }
        
        PostProcessTexture();
        
        // We did a synchronous load, must call AssetLoadCompleted here.
        assetAPI->AssetLoadCompleted(Name());
        return true;
    }
    catch(Ogre::Exception &e)
    {
        LogError("TextureAsset::DeserializeFromData: Failed to create texture " + this->Name().toStdString() + ": " + std::string(e.what()));
        return false;
    }
}

void TextureAsset::operationCompleted(Ogre::BackgroundProcessTicket ticket, const Ogre::BackgroundProcessResult &result)
{
    if (ticket != loadTicket_)
        return;
        
    // Reset to 0 to mark the asynch request is not active anymore. Aborted in Unload() if it is.
    loadTicket_ = 0;

    const QString assetRef = Name();
    ogreAssetName = AssetAPI::SanitateAssetRef(assetRef);
    if (!result.error)
    {
        ogreTexture = Ogre::TextureManager::getSingleton().getByName(ogreAssetName.toStdString(), OgreRenderer::OgreRenderingModule::CACHE_RESOURCE_GROUP);
        if (!ogreTexture.isNull())
        {
            PostProcessTexture();
            
            assetAPI->AssetLoadCompleted(assetRef);
            return;
        }
        else
            LogError("TextureAsset asynch load: Ogre::Texture was null after threaded loading: " + assetRef);
    }
    else
        LogError("TextureAsset asynch load: Ogre failed to do threaded loading: " + result.message);

    DoUnload();
    assetAPI->AssetLoadFailed(assetRef);
}

/*
void TextureAsset::RegenerateAllMipLevels()
{
    if (ogreTexture.isNull())
        return;

///\todo This function does not quite work, since ogreTexture->getNumMipmaps() will return 0 to denote a "full mipmap chain".

    for(int f = 0; f < ogreTexture->getNumFaces(); ++f)
        for(int i = 1; i < ogreTexture->getNumMipmaps(); ++i)
        {
            Ogre::HardwarePixelBufferSharedPtr src = ogreTexture->getBuffer(f, i-1);
            Ogre::Box srcSize(0, 0, src->getWidth(), src->getHeight());
            Ogre::HardwarePixelBufferSharedPtr dst = ogreTexture->getBuffer(f, i);
            Ogre::Box dstSize(0, 0, dst->getWidth(), dst->getHeight());
            dst->blit(src, srcSize, dstSize);
        }
}
*/
bool TextureAsset::SerializeTo(std::vector<u8> &data, const QString &serializationParameters) const
{
    PROFILE(TextureAsset_SerializeTo);
    if (ogreTexture.isNull())
    {
        LogWarning("SerializeTo: Called on an unloaded texture \"" + Name() + "\".");
        return false;
    }

    try
    {
        Ogre::Image newImage;
        ogreTexture->convertToImage(newImage);
        std::string formatExtension = serializationParameters.trimmed().toStdString();
        if (formatExtension.empty())
        {
            LogDebug("TextureAsset::SerializeTo: no serializationParameters given. Trying to guess format extension from the asset name.");
            formatExtension = QFileInfo(Name()).suffix().toStdString();
        }

        Ogre::DataStreamPtr imageStream = newImage.encode(formatExtension);
        if (imageStream.get() && imageStream->size() > 0)
        {
            data.resize(imageStream->size());
            imageStream->read(&data[0], data.size());
        }
    } catch(std::exception &e)
    {
        LogError("SerializeTo: Failed to export Ogre texture " + Name() + ":");
        if (e.what())
            LogError(e.what());
        return false;
    }
    return true;
}

void TextureAsset::DoUnload()
{
    // If a ongoing asynchronous asset load requested has been made to ogre, we need to abort it.
    // Otherwise Ogre will crash to our raw pointer that was passed if we get deleted. A ongoing ticket id cannot be 0.
    if (loadTicket_ != 0)
    {
        Ogre::ResourceBackgroundQueue::getSingleton().abortRequest(loadTicket_);
        loadTicket_ = 0;
    }
    
    if (!ogreTexture.isNull())
        ogreAssetName = ogreTexture->getName().c_str();

    ogreTexture = Ogre::TexturePtr();
    try
    {
        Ogre::TextureManager::getSingleton().remove(ogreAssetName.toStdString());
    }
    catch(...) {}
}

bool TextureAsset::IsLoaded() const
{
    return ogreTexture.get() != 0;
}

QImage TextureAsset::ToQImage(Ogre::Texture* tex, size_t faceIndex, size_t mipmapLevel)
{
    PROFILE(TextureAsset_ToQImage);
    if (!tex)
    {
        LogError("TextureAsset::ToQImage: Can't convert texture to QImage, null texture pointer");
        return QImage();
    }
    
    Ogre::Image ogreImage;
    tex->convertToImage(ogreImage);
    QImage::Format fmt;
    switch(ogreImage.getFormat())
    {
    case Ogre::PF_X8R8G8B8: fmt = QImage::Format_RGB32; break;
    case Ogre::PF_A8R8G8B8: fmt = QImage::Format_ARGB32; break;
    case Ogre::PF_R5G6B5: fmt = QImage::Format_RGB16; break;
    case Ogre::PF_R8G8B8: fmt = QImage::Format_RGB888; break;
    default:
        LogError("TextureAsset::ToQImage: Can't convert texture " + QString::fromStdString(tex->getName()) + " to QImage, unsupported image format " + QString::number(ogreImage.getFormat()));
        return QImage();
    }

    QImage img(ogreImage.getWidth(), ogreImage.getHeight(), fmt);
    assert(img.byteCount() == (int)ogreImage.getSize());
    memcpy(img.bits(), ogreImage.getData(), img.byteCount());

    return img;
}

QImage TextureAsset::ToQImage(size_t faceIndex, size_t mipmapLevel) const
{
    if (!ogreTexture.get())
    {
        LogError("TextureAsset::ToQImage: Can't convert texture to QImage, Ogre texture is not initialized for asset \"" + ToString() + "\"!");
        return QImage();
    }
    
    return ToQImage(ogreTexture.get(), faceIndex, mipmapLevel);
}

void TextureAsset::SetContentsFillSolidColor(int newWidth, int newHeight, u32 color, Ogre::PixelFormat ogreFormat, bool regenerateMipmaps, bool dynamic)
{
    if (newWidth == 0 || newHeight == 0)
    {
        Unload();
        return;
    }
    ///\todo Could optimize a lot here, don't create this temporary vector.
    ///\todo This only works for 32bpp images.
    std::vector<u32> data(newWidth * newHeight, color);
    SetContents(newWidth, newHeight, (const u8*)&data[0], data.size() * sizeof(u32), ogreFormat, regenerateMipmaps, dynamic, false);
}

void TextureAsset::SetContents(size_t newWidth, size_t newHeight, const u8 *data, size_t numBytes, Ogre::PixelFormat ogreFormat, bool regenerateMipMaps, bool dynamic, bool renderTarget)
{
    PROFILE(TextureAsset_SetContents);

    int usage = dynamic ? Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE : Ogre::TU_STATIC_WRITE_ONLY;
    if (regenerateMipMaps)
        usage |= Ogre::TU_AUTOMIPMAP;
    if (renderTarget)
        usage |= Ogre::TU_RENDERTARGET;

    if (numBytes != newWidth * newHeight * 4)
    {
        LogError("TextureAsset::SetContents failed: Inputted " + QString::number(numBytes) + " bytes of data, but " + QString::number(newWidth) + "x" + QString::number(newHeight)
            + " at 4 bytes per pixel requires " + QString::number(newWidth * newHeight * 4) + " bytes!");
        return;
    }

    if (!ogreTexture.get())
    {
        ogreTexture = Ogre::TextureManager::getSingleton().createManual(Name().toStdString(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D,
            newWidth, newHeight, regenerateMipMaps ? Ogre::MIP_UNLIMITED : 0, ogreFormat, usage);
        if (!ogreTexture.get())
        {
            LogError("TextureAsset::SetContents failed: Cannot create texture asset \"" + ToString() + "\" to name \"" + Name() + "\" and size " + QString::number(newWidth) + "x" + QString::number(newHeight) + "!");
            return;
        }
    }

    bool needRecreate = (newWidth != ogreTexture->getWidth() || newHeight != ogreTexture->getHeight() || ogreFormat != ogreTexture->getFormat());
//    if (newWidth == ogreTexture->getWidth() && newHeight == ogreTexture->getHeight() && ogreFormat == ogreTexture->getFormat())
//        return;

    if (needRecreate)
    {
        ogreTexture->freeInternalResources(); 
        ogreTexture->setWidth(newWidth);
        ogreTexture->setHeight(newHeight);
        ogreTexture->setFormat(ogreFormat);
        ogreTexture->createInternalResources();
    }
    if (ogreTexture->getBuffer().isNull())
    {
        LogError("TextureAsset::SetContents: Failed to create texture " + this->Name() + ": OgreTexture::getBuffer() was null!");
        return;
    }

    if (data)
    {
/*
#if defined(DIRECTX_ENABLED) && defined(WIN32)
        Ogre::HardwarePixelBufferSharedPtr pb = ogreTexture->getBuffer();
        Ogre::D3D9HardwarePixelBuffer *pixelBuffer = dynamic_cast<Ogre::D3D9HardwarePixelBuffer*>(pb.get());
        assert(pixelBuffer);

        LPDIRECT3DSURFACE9 surface = pixelBuffer->getSurface(Ogre::D3D9RenderSystem::getActiveD3D9Device());
        if (surface)
        {
            D3DSURFACE_DESC desc;
            HRESULT hr = surface->GetDesc(&desc);
            if (SUCCEEDED(hr))
            {
                D3DLOCKED_RECT lock;
                HRESULT hr = surface->LockRect(&lock, 0, 0);
                if (SUCCEEDED(hr))
                {
                    const int bytesPerPixel = 4; ///\todo Count from Ogre::PixelFormat!
                    const int sourceStride = bytesPerPixel * newWidth;
                    if (lock.Pitch == sourceStride)
                        memcpy(lock.pBits, data, sourceStride * newHeight);
                    else
                        for(size_t y = 0; y < newHeight; ++y)
                            memcpy((u8*)lock.pBits + lock.Pitch * y, data + sourceStride * y, sourceStride);
                    surface->UnlockRect();
                }
            }
        }
#else        
        */
        ///\todo Review Ogre internals of whether the const_cast here is safe!
        Ogre::PixelBox pixelBox(Ogre::Box(0,0, newWidth, newHeight), ogreFormat, const_cast<u8*>(data));
        ogreTexture->getBuffer()->blitFromMemory(pixelBox);
//#endif
    }
}

void TextureAsset::SetContentsDrawText(int newWidth, int newHeight, QString text, const QColor &textColor, const QFont &font, const QBrush &backgroundBrush, const QPen &borderPen, int flags, bool generateMipmaps, bool dynamic,
                                       float xRadius, float yRadius)
{
    PROFILE(TextureAsset_SetContentsDrawText);
    text = text.replace("\\n", "\n");

    // Create transparent pixmap
    QImage image(newWidth, newHeight, QImage::Format_ARGB32);
    image.fill(textColor.rgb() & 0x00FFFFFF);

    {
        // Init painter with pixmap as the paint device
        QPainter painter(&image);

        // Ask painter the rect for the text
        painter.setFont(font);
        QRect rect = painter.boundingRect(image.rect(), flags, text);

        // Set background brush
        painter.setBrush(backgroundBrush);
        painter.setPen(borderPen);
  
        painter.drawRoundedRect(rect, xRadius, yRadius, Qt::RelativeSize);
        
        // Draw text
        painter.setPen(textColor);
        painter.drawText(rect, flags, text);
    }

    SetContents(newWidth, newHeight, image.bits(), image.byteCount(), Ogre::PF_A8R8G8B8, generateMipmaps, dynamic, false);
}

void TextureAsset::PostProcessTexture()
{
    if (assetAPI->GetFramework()->HasCommandLineParameter("--autodxtcompress"))
        CompressTexture();
    if (assetAPI->GetFramework()->HasCommandLineParameter("--maxtexturesize"))
        ReduceTextureSize();
}

void TextureAsset::CompressTexture()
{
#if defined(DIRECTX_ENABLED) && defined(WIN32)
    if (ogreTexture.isNull())
        return;
    
    QStringList sizeParam = assetAPI->GetFramework()->CommandLineParameters("--maxtexturesize");
    size_t maxTextureSize = 0;
    if (sizeParam.size() > 0)
    {
        int size = sizeParam.first().toInt();
        if (size > 0)
            maxTextureSize = size;
    }
    
    Ogre::PixelFormat sourceFormat = ogreTexture->getFormat();
    if (sourceFormat >= Ogre::PF_DXT1 && sourceFormat <= Ogre::PF_DXT5)
        return; // Already compressed, do nothing
    if ((sourceFormat >= Ogre::PF_L8 && sourceFormat <= Ogre::PF_BYTE_LA) || sourceFormat == Ogre::PF_R8)
        return; // 1 or 2 byte format, leave alone
    
    PROFILE(TextureAsset_CompressTexture);
    
    // Ogre will crash on OpenGL if it tries to get texture data. Therefore perform a no-op on OpenGL
    /// \todo Fix the crash
    // (note: on OpenGL memory use of the Tundra process is less critical anyway, as the system memory copy
    // of the texture is contained by the display driver)
    if (Ogre::Root::getSingletonPtr()->getRenderSystem()->getName() == "OpenGL Rendering Subsystem")
    {
        LogWarning("Skipping CompressTexture on OpenGL as it is prone to crash");
        return;
    }
    
    // Get original texture data
    std::vector<unsigned char*> imageData;
    std::vector<Ogre::PixelBox> imageBoxes;
    
    size_t numMipmaps = ogreTexture->getNumMipmaps();
    
    for (size_t level = 0; level <= numMipmaps; ++level)
    {
        try
        {
            Ogre::HardwarePixelBufferSharedPtr buf = ogreTexture->getBuffer(0, level);
            
            // If a max texture size is set, and mipmaps are present in the source texture, reject those larger than acceptable texture size, however ensure at least 1 mipmap
            if (maxTextureSize > 0 && numMipmaps > 0)
            {
                if (level < numMipmaps && imageBoxes.size() == 0 && (buf->getWidth() > maxTextureSize || buf->getHeight() > maxTextureSize))
                    continue;
            }
            
            unsigned char* levelData = new unsigned char[buf->getWidth() * buf->getHeight() * 4];
            imageData.push_back(levelData);
            Ogre::PixelBox levelBox(Ogre::Box(0, 0, buf->getWidth(), buf->getHeight()), Ogre::PF_A8B8G8R8, levelData);
            buf->blitToMemory(levelBox);
            imageBoxes.push_back(levelBox);
        }
        catch (std::exception& e)
        {
            LogError("TextureAsset::CompressTexture: Caught exception " + QString(e.what()) + " while handling miplevel " + QString::number(level) + ", aborting.");
            break;
        }
    }
    
    // If we only have 1 mipmap, and it is too large, resample it now
    if (maxTextureSize > 0 && imageBoxes.size() == 1 && (imageBoxes[0].right > maxTextureSize || imageBoxes[0].bottom > maxTextureSize))
    {
        size_t targetWidth = imageBoxes[0].right;
        size_t targetHeight = imageBoxes[0].bottom;
        while (targetWidth > maxTextureSize || targetHeight > maxTextureSize)
        {
            targetWidth >>= 1;
            targetHeight >>= 1;
        }
        if (!targetWidth)
            targetWidth = 1;
        if (!targetHeight)
            targetHeight = 1;
        
        unsigned char* scaledPixelData = new unsigned char[targetWidth * targetHeight * 4];
        Ogre::PixelBox targetBox(Ogre::Box(0, 0, targetWidth, targetHeight), Ogre::PF_A8B8G8R8, scaledPixelData);
        Ogre::Image::scale(imageBoxes[0], targetBox);
        
        // Delete the unscaled original data and replace the original pixelbox
        delete[] imageData[0];
        imageData[0] = scaledPixelData;
        imageBoxes[0] = targetBox;
    }
    
    // Determine format
    int flags = squish::kColourRangeFit; // Lowest quality, but fastest
    size_t bytesPerBlock = 8;
    Ogre::PixelFormat newFormat = Ogre::PF_DXT1;
    if (ogreTexture->hasAlpha())
    {
        LogDebug("CompressTexture " + Name() + " image format " + QString::number(sourceFormat) + ", compressing as DXT5");
        newFormat = Ogre::PF_DXT5;
        bytesPerBlock = 16;
        flags |= squish::kDxt5;
    }
    else
    {
        LogDebug("CompressTexture " + Name() + " image format " + QString::number(sourceFormat) + ", compressing as DXT1");
        flags |= squish::kDxt1;
    }
    
    // Compress original texture data
    std::vector<unsigned char*> compressedImageData;
    
    for (size_t level = 0; level < imageBoxes.size(); ++level)
    {
        int compressedSize = squish::GetStorageRequirements(imageBoxes[level].right, imageBoxes[level].bottom, flags);
        LogDebug("Compressing level " + QString::number(level) + " " + QString::number(imageBoxes[level].right) + "x" + QString::number(imageBoxes[level].bottom) + " into " + QString::number(compressedSize) + " bytes");
        unsigned char* compressedData = new unsigned char[compressedSize];
        squish::CompressImage((squish::u8*)imageData[level], imageBoxes[level].right, imageBoxes[level].bottom, compressedData, flags);
        compressedImageData.push_back(compressedData);
    }
    
    // Change Ogre texture format
    ogreTexture->freeInternalResources();
    ogreTexture->setWidth(imageBoxes[0].right);
    ogreTexture->setHeight(imageBoxes[0].bottom);
    ogreTexture->setFormat(newFormat);
    ogreTexture->setNumMipmaps(imageBoxes.size() - 1);
    ogreTexture->createInternalResources();
    
    // Upload compressed texture data
    for (size_t level = 0; level < imageBoxes.size(); ++level)
    {
        try
        {
            Ogre::HardwarePixelBufferSharedPtr buf = ogreTexture->getBuffer(0, level);
            
            // Ogre does not load the texture data properly if the miplevel width is not divisible by 4, so we write manual code for Direct3D9
            /// \todo Fix bug in ogre-safe-nocrashes branch. It also affects Ogre's loading of already compressed DDS files
            
            size_t numRows = (buf->getHeight() + 3) / 4;
            int sourceStride = (buf->getWidth() + 3) / 4 * bytesPerBlock;
            unsigned char* src = compressedImageData[level];
            
            Ogre::D3D9HardwarePixelBuffer *pixelBuffer = dynamic_cast<Ogre::D3D9HardwarePixelBuffer*>(buf.get());
            assert(pixelBuffer);
            LPDIRECT3DSURFACE9 surface = pixelBuffer->getSurface(Ogre::D3D9RenderSystem::getActiveD3D9Device());
            if (surface)
            {
                D3DLOCKED_RECT lock;
                HRESULT hr = surface->LockRect(&lock, 0, 0);
                if (SUCCEEDED(hr))
                {
                    if (lock.Pitch == sourceStride)
                        memcpy(lock.pBits, src, sourceStride * numRows);
                    else
                        for(size_t y = 0; y < numRows; ++y)
                            memcpy((u8*)lock.pBits + lock.Pitch * y, src + sourceStride * y, sourceStride);
                    surface->UnlockRect();
                }
            }
        }
        catch (std::exception& e)
        {
            LogError("TextureAsset::CompressTexture: Caught exception " + QString(e.what()) + " while handling miplevel " + QString::number(level) + ", aborting.");
            break;
        }
    }
    
    // Delete CPU-side temp image data
    for (size_t i = 0; i < imageData.size(); ++i)
        delete[] imageData[i];
    for (size_t i = 0; i < compressedImageData.size(); ++i)
        delete[] compressedImageData[i];
#endif
}

void TextureAsset::ReduceTextureSize()
{
#if defined(DIRECTX_ENABLED) && defined(WIN32)
    if (ogreTexture.isNull())
        return;
    
    QStringList sizeParam = assetAPI->GetFramework()->CommandLineParameters("--maxtexturesize");
    size_t maxTextureSize = 0;
    if (sizeParam.size() > 0)
    {
        int size = sizeParam.first().toInt();
        if (size > 0)
            maxTextureSize = size;
    }
    if (!maxTextureSize)
        return;
    
    if (ogreTexture->getWidth() <= maxTextureSize && ogreTexture->getHeight() <= maxTextureSize)
        return; // Texture OK, no reduction needed
    
    PROFILE(TextureAsset_ReduceTextureSize);
    
    size_t origWidth = ogreTexture->getWidth();
    size_t origHeight = ogreTexture->getHeight();
    
    if (Ogre::Root::getSingletonPtr()->getRenderSystem()->getName() == "OpenGL Rendering Subsystem")
    {
        LogWarning("Skipping ReduceTextureSize on OpenGL as it is prone to crash");
        return;
    }
    
    Ogre::PixelFormat sourceFormat = ogreTexture->getFormat();
    
    // Get original texture data
    std::vector<unsigned char*> imageData;
    std::vector<Ogre::PixelBox> imageBoxes;
    
    size_t numMipmaps = ogreTexture->getNumMipmaps();
    
    // If mipmaps are present in the source texture, reject those larger than acceptable texture size, however ensure at least 1 mipmap
    for (size_t level = 0; level <= numMipmaps; ++level)
    {
        try
        {
            Ogre::HardwarePixelBufferSharedPtr buf = ogreTexture->getBuffer(0, level);
            
            if (numMipmaps > 0 && level < numMipmaps && imageBoxes.size() == 0 && (buf->getWidth() > maxTextureSize || buf->getHeight() > maxTextureSize))
                continue;
            
            unsigned char* levelData = new unsigned char[Ogre::PixelUtil::getMemorySize(buf->getWidth(), buf->getHeight(), 1, buf->getFormat())];
            imageData.push_back(levelData);
            Ogre::PixelBox levelBox(Ogre::Box(0, 0, buf->getWidth(), buf->getHeight()), buf->getFormat(), levelData);
            
            if (sourceFormat >= Ogre::PF_DXT1 && sourceFormat <= Ogre::PF_DXT5)
            {
                // Ogre does not retrieve the texture data properly if the miplevel width is not divisible by 4, so we write manual code for Direct3D9
                /// \todo Fix bug in ogre-safe-nocrashes branch
                size_t bytesPerBlock = sourceFormat == Ogre::PF_DXT1 ? 8 : 16;
                size_t numRows = (buf->getHeight() + 3) / 4;
                int destStride = (buf->getWidth() + 3) / 4 * bytesPerBlock;
                unsigned char* dest = levelData;
                
                Ogre::D3D9HardwarePixelBuffer *pixelBuffer = dynamic_cast<Ogre::D3D9HardwarePixelBuffer*>(buf.get());
                assert(pixelBuffer);
                LPDIRECT3DSURFACE9 surface = pixelBuffer->getSurface(Ogre::D3D9RenderSystem::getActiveD3D9Device());
                if (surface)
                {
                    D3DLOCKED_RECT lock;
                    HRESULT hr = surface->LockRect(&lock, 0, 0);
                    if (SUCCEEDED(hr))
                    {
                        if (lock.Pitch == destStride)
                            memcpy(dest, lock.pBits, destStride * numRows);
                        else
                            for(size_t y = 0; y < numRows; ++y)
                                memcpy(dest + destStride * y, (u8*)lock.pBits + lock.Pitch * y, destStride);
                        surface->UnlockRect();
                    }
                }
            }
            else
                buf->blitToMemory(levelBox);
            
            imageBoxes.push_back(levelBox);
        }
        catch (std::exception& e)
        {
            LogError("TextureAsset::ReduceTextureSize: Caught exception " + QString(e.what()) + " while handling miplevel " + QString::number(level) + ", aborting.");
            break;
        }
    }
    
    // If only one level, resize it. But do not attempt this for textures which already are DXT compressed
    if (imageBoxes.size() == 1 && (imageBoxes[0].right > maxTextureSize || imageBoxes[0].bottom > maxTextureSize))
    {
        if (sourceFormat >= Ogre::PF_DXT1 && sourceFormat <= Ogre::PF_DXT5)
        {
            LogWarning("TextureAsset::ReduceTextureSize: not resizing already DDS compressed texture " + Name());
            for (size_t i = 0; i < imageData.size(); ++i)
                delete[] imageData[i];
            return;
        }
        
        size_t targetWidth = imageBoxes[0].right;
        size_t targetHeight = imageBoxes[0].bottom;
        while (targetWidth > maxTextureSize || targetHeight > maxTextureSize)
        {
            targetWidth >>= 1;
            targetHeight >>= 1;
        }
        if (!targetWidth)
            targetWidth = 1;
        if (!targetHeight)
            targetHeight = 1;
        
        unsigned char* scaledPixelData = new unsigned char[Ogre::PixelUtil::getMemorySize(targetWidth, targetHeight, 1, sourceFormat)];
        Ogre::PixelBox targetBox(Ogre::Box(0, 0, targetWidth, targetHeight), sourceFormat, scaledPixelData);
        Ogre::Image::scale(imageBoxes[0], targetBox);
        
        // Delete the unscaled original data and replace the original pixelbox
        delete[] imageData[0];
        imageData[0] = scaledPixelData;
        imageBoxes[0] = targetBox;
    }
    
    // Change texture parameters
    ogreTexture->freeInternalResources();
    ogreTexture->setWidth(imageBoxes[0].right);
    ogreTexture->setHeight(imageBoxes[0].bottom);
    ogreTexture->setNumMipmaps(imageBoxes.size() - 1);
    ogreTexture->createInternalResources();
    
    // Upload new texture data
    for (size_t level = 0; level < imageBoxes.size(); ++level)
    {
        try
        {
            Ogre::HardwarePixelBufferSharedPtr buf = ogreTexture->getBuffer(0, level);
            
            if (sourceFormat >= Ogre::PF_DXT1 && sourceFormat <= Ogre::PF_DXT5)
            {
                // Ogre does not load the texture data properly if the miplevel width is not divisible by 4, so we write manual code for Direct3D9
                /// \todo Fix bug in ogre-safe-nocrashes branch. It also affects Ogre's loading of already compressed DDS files
                size_t bytesPerBlock = sourceFormat == Ogre::PF_DXT1 ? 8 : 16;
                size_t numRows = (buf->getHeight() + 3) / 4;
                int sourceStride = (buf->getWidth() + 3) / 4 * bytesPerBlock;
                unsigned char* src = imageData[level];
                
                Ogre::D3D9HardwarePixelBuffer *pixelBuffer = dynamic_cast<Ogre::D3D9HardwarePixelBuffer*>(buf.get());
                assert(pixelBuffer);
                LPDIRECT3DSURFACE9 surface = pixelBuffer->getSurface(Ogre::D3D9RenderSystem::getActiveD3D9Device());
                if (surface)
                {
                    D3DLOCKED_RECT lock;
                    HRESULT hr = surface->LockRect(&lock, 0, 0);
                    if (SUCCEEDED(hr))
                    {
                        if (lock.Pitch == sourceStride)
                            memcpy(lock.pBits, src, sourceStride * numRows);
                        else
                            for(size_t y = 0; y < numRows; ++y)
                                memcpy((u8*)lock.pBits + lock.Pitch * y, src + sourceStride * y, sourceStride);
                        surface->UnlockRect();
                    }
                }
            }
            else
                buf->blitFromMemory(imageBoxes[level]);
        }
        catch (std::exception& e)
        {
            LogError("TextureAsset::ReduceTextureSize: Caught exception " + QString(e.what()) + " while handling miplevel " + QString::number(level) + ", aborting.");
            break;
        }
    }
    
    // Delete CPU-side temp image data
    for (size_t i = 0; i < imageData.size(); ++i)
        delete[] imageData[i];
    
    LogDebug("TextureAsset::ReduceTextureSize: asset " + Name() + " reduced from " + QString::number(origWidth) + "x" + QString::number(origHeight) + " to " + QString::number(ogreTexture->getWidth()) + "x" + QString::number(ogreTexture->getHeight()));
#endif
}

