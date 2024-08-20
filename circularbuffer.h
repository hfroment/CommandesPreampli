#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

template <class T>
class CircularBuffer
{
public:
    explicit CircularBuffer(size_t size) :
        buf_(new T[size]),
        max_size_(size),
        head_(0),
        tail_(0),
        full_(false)
    {
    }

    void put(T item)
    {
        noInterrupts();

        buf_[head_] = item;
        if(full_)
        {
            tail_ = (tail_ + 1) % max_size_;
        }
        head_ = (head_ + 1) % max_size_;
        full_ = head_ == tail_;

        interrupts();
    }

    T get()
    {
        noInterrupts();

        if(empty())
        {
            interrupts();
            return T();
        }
        //Read data and advance the tail (we now have a free space)
        auto val = buf_[tail_];
        full_ = false;
        tail_ = (tail_ + 1) % max_size_;

        interrupts();

        return val;
    }

    void reset()
    {
        noInterrupts();

        head_ = tail_;
        full_ = false;
        interrupts();
    }

    bool empty() const
    {
        //if head and tail are equal, we are empty
        return (!full_ && (head_ == tail_));
    }

    bool full() const
    {
        //If tail is ahead the head by 1, we are full
        return full_;
    }

    size_t capacity() const
    {
        return max_size_;
    }

    size_t size() const
    {
        size_t size = max_size_;

        if(!full_)
        {
            if(head_ >= tail_)
            {
                size = head_ - tail_;
            }
            else
            {
                size = max_size_ + head_ - tail_;
            }
        }

        return size;
    }

private:
    T* buf_;
    size_t head_;
    size_t tail_;
    const size_t max_size_;
    bool full_;
};
#endif // CIRCULARBUFFER_H
