/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   AudioSignalLabel.h
 *  @brief  Shows audio signal's wave form.
 */

#pragma once

#include "CoreTypes.h"

#include <QLabel>

// Shows audio signal's wave form.
/** Label will resize the signal image when it's size is changed.
    @cond PRIVATE */
class AudioSignalLabel: public QLabel
{
    Q_OBJECT

public:
    AudioSignalLabel(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    virtual ~AudioSignalLabel();
    /// Set new audio data into label.
    void SetAudioData(std::vector<u8> &data, uint frequency, uint bits, bool stereo);
    /// Get audio clip durtation in seconds.
    float GetAudioDuration() const;

private slots:
    /// Generate new audio signal using soundBuffer information.
    /// @todo add support for stereo sound drawing, can't test it because all uploaded sounds are now converted into Mono format.
    void GenerateAudioSignalImage();

protected:
    /// Raise a flag when window is resized so that we know when we need to repaint the window.
    virtual void resizeEvent(QResizeEvent *ev);
    /// If widget's size has recently been updated redraw the labell's image.
    virtual void paintEvent(QPaintEvent *ev);

private:
    /// Audio data vector.
    std::vector<u8> audioData_;
    /// What is audio file's frequency
    uint frequency_;
    /// How many bits is used per audio sample.
    uint bitsPerSample_;
    /// Is audio file using stereo sound.
    bool stereo_;
    /// Audio clip duration in seconds.
    float duration_;
    /// Is widget resized
    bool widget_resized_;
};
/** @endcond */
