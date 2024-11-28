export module sr_harray;

import std;

using std::unique_ptr;
using std::span;
using std::copy;

export namespace sr
{
    template<typename T>
    struct Harray
    {
    private:

        unique_ptr<T[]> memory;
        span<T> elements;

    public:

        Harray() = default;

        Harray(size_t size):
            memory { new T[size] },
            elements { memory.get(), size }
        { }

        Harray(span<T> elements):
            memory { new T[elements.size()] },
            elements { memory.get(), elements.size() }
        {
            copy(elements.begin(), elements.end(), this->elements.begin());
        }

        Harray(const Harray& other): Harray(other.elements) { }
        Harray& operator=(const Harray& other)
        {
            if (!memory || this->size() != other.size())
            {
                memory.reset(new bool[other.size()]);
                elements = { memory.get(), other.size() };
            }

            copy(
                other.get_elements().begin(),
                other.get_elements().end(),
                elements.begin()
            );

            return *this;
        }

        Harray(Harray&&) = default;
        Harray& operator=(Harray&&) = default;

        inline T* get_data() const noexcept
        {
            return memory.get();
        }

        inline span<T> get_elements() const noexcept
        {
            return elements;
        }

        inline size_t size() const noexcept
        {
            return elements.size();
        }

        T& operator[](size_t idx)
        {
            return elements[idx];
        }

        const T& operator[](size_t idx) const
        {
            return elements[idx];
        }
    };
}
