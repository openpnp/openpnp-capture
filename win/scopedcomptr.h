/*

    Scoped COM pointer to auto-release the object
    when it goes out of scope

    Niels A. Moseley

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