// For conditions of distribution and use, see copyright notice in license.txt

#include "IDevice.h"

IDevice::IDevice(const QString &name) :
    name_(name),
    running_(false)
{
}

IDevice::~IDevice()
{
    emit Destroyed();
}

void IDevice::Start()
{
    if (IsRunning())
        return;

    running_ = InternalStart();
}

void IDevice::Stop()
{
    if (!IsRunning())
        return;

    running_ = !InternalStop();
}

bool IDevice::IsRunning()
{
    return running_;
}

QString IDevice::Name()
{
    return name_;
}