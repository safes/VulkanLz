/***************************************************************************************************
 * Copyright (c) 2010-2023, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************************************/
/// \file
/// \brief Pointer type.

#ifndef MI_NEURAYLIB_IPOINTER_H
#define MI_NEURAYLIB_IPOINTER_H

#include <mi/neuraylib/idata.h>

namespace mi {

/** \addtogroup mi_neuray_simple_types
@{
*/

/// This interface represents mutable pointers.
///
/// Mutable pointers in the sense of this interface are mutable interface pointers, not \c void*
/// pointers.
///
/// Pointers are either typed or untyped. Typed pointers enforce the target to be of a certain type
/// derived from #mi::IData. The type name of a typed mutable pointer is \c "Pointer<", followed by
/// the type name of the target, and finally \c ">", e.g., \c "Pointer<Sint32>" for a mutable
/// pointer to an instance of #mi::ISint32.
///
/// Untyped pointers simply store a mutable pointer of type #mi::base::IInterface. The type name of
/// an untyped mutable pointer is \c "Pointer<Interface>".
///
/// The additional level of indirection can be used to use mutable interface pointers with data
/// collections while maintaining const correctness. E.g., the getters of #mi::IData_collection,
/// #mi::IArray, #mi::IDynamic_array, and #mi::IMap return const interface pointers if the instance
/// itself is const. If you want to retrieve a mutable interface pointer from such a collection
/// you have to wrap it in an instance of #mi::IPointer before you store it. When you retrieve it
/// from the const collection, you can call #get_pointer() to obtain the wrapped mutable interface
/// pointer.
///
/// \see #mi::IConst_pointer.
class IPointer :
    public base::Interface_declare<0xd921b94b,0x0b64,0x4da0,0x97,0x95,0xdc,0x4d,0xaf,0x99,0x95,0xd5,
                                   IData_simple>
{
public:
    /// Sets the pointer.
    ///
    /// Note that a \c NULL pointer is a valid parameter value that clears the previously set
    /// pointer. Subsequent #get_pointer() calls will return \c NULL then.
    ///
    /// \return
    ///                -  0: Success.
    ///                - -1: \p pointer has the wrong type.
    virtual Sint32 set_pointer( base::IInterface* pointer) = 0;

    /// Returns the pointer.
    virtual base::IInterface* get_pointer() const = 0;

    /// Returns the pointer.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \tparam T     The interface type of the element to return
    template<class T>
    T* get_pointer() const
    {
        base::IInterface* ptr_iinterface = get_pointer();
        if ( !ptr_iinterface)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }
};

/// This interface represents const pointers.
///
/// Const pointers in the sense of this interface are const interface pointers, not \c const
/// \c void* pointers.
///
/// Pointers are either typed or untyped. Typed pointers enforce the target to be of a certain type
/// derived from #mi::IData. The type name of a typed const pointer is \c "Const_pointer<", followed
/// by the type name of the target, and finally \c ">", e.g., \c "Const_pointer<Sint32>" for a
/// const pointer to an instance of #mi::ISint32.
///
/// Untyped pointers simply store a const pointer of type #mi::base::IInterface. The type name of an
/// untyped const pointer is \c "Const_pointer<Interface>".
///
/// The additional level of indirection can be used to use const interface pointers with data
/// collections while maintaining const correctness. E.g., the setters of #mi::IData_collection,
/// #mi::IArray, #mi::IDynamic_array, and #mi::IMap take mutable interface pointers. If you want
/// to store a const interface pointer you have to wrap it in an instance of #mi::IConst_pointer
/// before using it with these interfaces.
///
/// \see #mi::IPointer.
class IConst_pointer :
    public base::Interface_declare<0x67bfc3ef,0x7d18,0x406e,0x95,0x3b,0x98,0xe6,0xb2,0x98,0x93,0x39,
                                   IData_simple>
{
public:
    /// Sets the const pointer.
    ///
    /// Note that a \c NULL pointer is a valid parameter value that clears the previously set
    /// pointer. Subsequent #get_pointer() calls will return \c NULL then.
    ///
    /// \return
    ///                -  0: Success.
    ///                - -1: \p pointer has the wrong type.
    virtual Sint32 set_pointer( const base::IInterface* pointer) = 0;

    /// Returns the const pointer.
    virtual const base::IInterface* get_pointer() const = 0;

    /// Returns the const pointer.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \tparam T     The interface type of the element to return
    template<class T>
    const T* get_pointer() const
    {
        const base::IInterface* ptr_iinterface = get_pointer();
        if ( !ptr_iinterface)
            return 0;
        const T* ptr_T = static_cast<const T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }
};

/**@}*/ // end group mi_neuray_simple_types

} // namespace mi

#endif // MI_NEURAYLIB_IPOINTER_H
