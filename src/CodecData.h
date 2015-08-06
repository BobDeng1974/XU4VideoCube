#pragma once

#include "Exception.h"

#include <memory>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <vector>


class Buffer
{
public:
    size_t Offset = 0;
    void* Address = 0;
    size_t Length = 0;
};

class CodecData
{
public:
    int Index;
    int  planesCount;

protected:
    std::vector<std::shared_ptr<Buffer>> planes;


    CodecData(int  planesCount)
        : planesCount(planesCount)
    {
        //planes.reserve(planesCount);

        for (int i = 0; i < planesCount; ++i)
        {
            planes.push_back(std::shared_ptr<Buffer>(new Buffer()));
        }
    }


public:
    virtual ~CodecData()
    {
    }


    template<typename T>
    static std::vector<std::shared_ptr<T>> RequestBuffers(int fd, v4l2_buf_type type, v4l2_memory memory, int count, bool enqueue) //where T: CodecData, new()
    {
        // Reqest input buffers
        v4l2_requestbuffers reqbuf = {0};
        reqbuf.count = (uint)count;
        reqbuf.type = (uint)type;
        reqbuf.memory = (uint)memory;

        if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf) != 0)
            throw Exception("VIDIOC_REQBUFS failed.");


        // Get the buffer details
        std::vector<std::shared_ptr<T>> inBuffers;
        //inBuffers.reserve(reqbuf.count);    // the actual count may differ from requested count


        for (int n = 0; n < (int)reqbuf.count; ++n)
        {
            T* temp = new T();

            std::shared_ptr<T> codecData(temp);

            v4l2_plane planes[codecData->planesCount] = {0};


            v4l2_buffer buf = {0};
            buf.type = (uint)type;
            buf.memory = (uint)memory;
            buf.index = (uint)n;
            buf.m.planes = planes;
            buf.length = (uint)codecData->planesCount;

            int ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
            if (ret != 0)
            {
                throw Exception("VIDIOC_QUERYBUF failed.");
            }

            //inBuffers[n] = codecData;
            inBuffers.push_back(codecData);

            for (int i = 0; i < codecData->planesCount; ++i)
            {
                codecData->Index = n;

                codecData->planes[i]->Offset = planes[i].m.mem_offset;

                codecData->planes[i]->Address = mmap(nullptr, planes[i].length,
                                                     PROT_READ | PROT_WRITE,
                                                     MAP_SHARED,
                                                     fd,
                                                     (int)planes[i].m.mem_offset);

                codecData->planes[i]->Length = planes[i].length;
            }

            if (enqueue)
            {
                // Enqueu buffer
                int ret = ioctl(fd, (uint)VIDIOC_QBUF, &buf);
                if (ret != 0)
                {
                    throw Exception("VIDIOC_QBUF failed.");
                }
            }
        }


        return inBuffers;
    }
};

class MediaStreamCodecData : public CodecData
{
public:
    std::shared_ptr<Buffer> GetBuffer() const
    {
        return planes[0];
    }

    MediaStreamCodecData()
        : CodecData(1)
    {
    }
};

class NV12CodecData : public CodecData
{
public:
    std::shared_ptr<Buffer> GetY() const
    {
        return planes[0];
    }

    std::shared_ptr<Buffer> GetVU() const
    {
        return planes[1];
    }

    NV12CodecData()
        : CodecData(2)
    {
    }
};
