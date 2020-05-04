/*

    OpenPnp-Capture: a video capture subsystem.

    Scoped COM pointer to auto-release the object
    when it goes out of scope

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

#ifndef win_scopedcomptr_h
#define win_scopedcomptr_h

/** The ScopedComPtr takes a pointer to a generic COM
    interface object and will release the object when
    the owning ScopedComPtr is released/destroyed.

    Example:
    sampleGrabberPointer = .. create ISampleGrabber via COM
    ScopedComPtr<ISampleGrabber> sampleGrabber(sampleGrabberPointer);

    .. code .. 
    if (error)
    {
        return; // this will automatically release ISampleGrabber
    }

    If the pointer handled by ScopedComPtr needs to be returned
    from a function, the Detach() method will transfer ownership.

*/
template <typename Interface> class ScopedComPtr
{
public:
    /** create a ScopedComPtr object and take ownership of
        the Interface object/pointer */
    ScopedComPtr(Interface* ptr) : m_ptr(ptr)
    {
        
    }

    /** destroy the ScopedComPtr object and
        release the associated Interface pointer */
    ~ScopedComPtr()
    {
        if (m_ptr != nullptr)
        {
            InternalRelease();
        }
    }

    /** access methods exposed by Interface */
    Interface* operator->() const
    {
        return m_ptr;
    }    

    /** replace the internal pointer with a new one,
        the old object is released/destroyed */
    void Replace(Interface *newPtr)
    {
        InternalRelease();
        m_ptr = newPtr;
    }

    /** detach the object from the scoped pointer object
        without releasing it. */
    Interface* Detach()
    {
        Interface *obj = m_ptr;
        m_ptr = nullptr;
        return obj;
    }

private:
    void InternalRelease()
    {
        Interface * temp = m_ptr;
        if (temp != nullptr)
        {
            m_ptr = nullptr;
            temp->Release();
        }
    }

    //disallow copying of a scoped pointer!
    ScopedComPtr(ScopedComPtr const & other)
    {

    }

    Interface *m_ptr = nullptr;    
};

/** SafeRelease(ptr) check if the ptr isn't NULL.
    If it isn't it invokes the Release() method on it */
template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#endif