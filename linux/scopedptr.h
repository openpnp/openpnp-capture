/*

    OpenPnp-Capture: a video capture subsystem.

    Scoped pointer to auto-delete the object when it goes out of scope

    Copyright (c) 2017 Niels Moseley.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
    
*/

#ifndef linux_scopedptr_h
#define linux_scopedptr_h

/** The ScopedPtr takes a pointer to a generic object and will 
    delete/destroy the object when the owning ScopedComPtr is 
    released/destroyed.

    If the pointer handled by ScopedComPtr needs to be returned
    from a function, the Detach() method will transfer ownership.

*/
template <typename T> class ScopedPtr
{
public:
    /** create a ScopedComPtr object and take ownership of
        the Interface object/pointer */
    ScopedPtr(T* ptr) : m_ptr(ptr)
    {
        
    }

    /** destroy the ScopedComPtr object and
        release the associated Interface pointer */
    ~ScopedPtr()
    {
        if (m_ptr != nullptr)
        {
            delete m_ptr;
        }
    }

    /** access methods exposed by Interface */
    T* operator->() const
    {
        return m_ptr;
    }    

    /** replace the internal pointer with a new one,
        the old object is released/destroyed */
    void Replace(T *newPtr)
    {
        InternalRelease();
        m_ptr = newPtr;
    }

    /** detach the object from the scoped pointer object
        without releasing it. */
    T* Detach()
    {
        T *obj = m_ptr;
        m_ptr = nullptr;
        return obj;
    }

private:
    void InternalRelease()
    {
        T * temp = m_ptr;
        if (temp != nullptr)
        {
            m_ptr = nullptr;
            delete temp;
        }
    }

    //disallow copying of a scoped pointer!
    ScopedPtr(ScopedPtr const & other)
    {

    }

    T *m_ptr = nullptr;    
};

#endif