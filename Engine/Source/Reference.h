#pragma once

#include "Defines.h"
#include <stdint.h>
#include <assert.h>
#include <unordered_map>

namespace Imzadi
{
	/**
	 * This is the base class for any dynamically allocated class that we would like to
	 * reference count.  Be careful not to inherit this class more than once.  I try to
	 * avoid multiple inheritance, but if it is done, use virtual inheritance to make
	 * sure that only one of these is inherited.  Otherwise, you could get a double-delete.
	 *
	 * This class works in conjunction with the Reference class.
	 *
	 * Note that it is possible to create a memory leak by creating a direct or indirect
	 * circular reference.  No attempt is made to catch or detect circular references.
	 * You can use handles instead of references to deal with the problem of circular references.
	 * Compare this mechanism to that of std::weak_ptr<>.
	 *
	 * Note that this class and the Reference class are not thread-safe.
	 */
	class IMZADI_API ReferenceCounted
	{
	public:
		/**
		 * Construct a new reference-counted object with a ref-count of zero.
		 * Add this object to the set of all referenced objects.
		 */
		ReferenceCounted();

		/**
		 * Remove this object from the set of all referenced objects.
		 */
		virtual ~ReferenceCounted();

		/**
		 * Increment our reference count.
		 */
		void IncRef() const;

		/**
		 * Decrement our reference count.  Warning: if the count goes from 1 to 0,
		 * then we delete the this-pointer before returning from the call!
		 */
		void DecRef() const;

		/**
		 * Return a handle that can be later used to get a pointer to this object
		 * if it has not yet been destroyed.  Note that a valid handle is always
		 * non-zero.
		 */
		uint32_t GetHandle() const { return this->handle; }

		/**
		 * Try to dereference the given handle into a reference-counted object.
		 * Null is returned if the handle is invalid or if the reference-counted
		 * object to which it once referred has already been destroyed.
		 *
		 * @param[in] handle This is the handle returned by GetHandle on the desired object instance.
		 * @return A pointer to the desired reference-counted object is returned, or null if it has gone out of scope.
		 */
		static ReferenceCounted* GetObjectFromHandle(uint32_t handle);

	private:
		mutable uint32_t refCount;		///< This is used to keep track of how many Reference class instances are pointing to this object.
		uint32_t handle;				///< This is used to track this object without holding onto a reference to the object.
		static uint32_t nextHandle;		///< A new object is assigned this handle.

		typedef std::unordered_map<uint32_t, ReferenceCounted*> ObjectMap;
		static ObjectMap objectMap;		///< The set of all referenced objects is maintained here for the purpose of handle dereferencing.
	};

	/**
	 * This class is my alternative to std::shared_ptr<>.  I don't claim to have
	 * written something better here, but it meets my needs in at least one way
	 * to which std::shared_ptr<> doesn't seem to be able.  That is, you can't safely
	 * convert from a raw C-pointer back to a std::shared_ptr<>.  You can, of course,
	 * convert from a std::shared_ptr<> to a C-pointer, but I need to be able to go
	 * the other direction without causing a double-delete.
	 */
	template<typename T>
	class IMZADI_API Reference
	{
	public:
		Reference()
		{
			this->refCounted = nullptr;
		}

		Reference(const Reference& ref)
		{
			this->refCounted = nullptr;
			this->Set(ref.refCounted);
		}

		Reference(ReferenceCounted* refCounted)
		{
			this->refCounted = nullptr;
			this->Set(refCounted);
		}

		virtual ~Reference()
		{
			this->Set(nullptr);
		}

		void operator=(const Reference& ref)
		{
			this->Set(ref.refCounted);
		}

		T* operator->()
		{
			return this->Get();
		}

		const T* operator->() const
		{
			return this->Get();
		}

		operator T* ()
		{
			return this->Get();
		}

		operator const T* () const
		{
			return this->Get();
		}

		T* Get()
		{
			return (T*)this->refCounted;
		}

		const T* Get() const
		{
			return (T*)const_cast<const ReferenceCounted*>(this->refCounted);
		}

		T* SafeGet()
		{
			return dynamic_cast<T*>(this->refCounted);
		}

		const T* SafeGet() const
		{
			return dynamic_cast<const T*>(const_cast<const ReferenceCounted*>(this->refCounted));
		}

		void Reset()
		{
			this->Set(nullptr);
		}

		void Set(ReferenceCounted* refCounted)
		{
			if (this->refCounted)
				this->refCounted->DecRef();

			this->refCounted = refCounted;

			if (this->refCounted)
				this->refCounted->IncRef();
		}

		void SafeSet(ReferenceCounted* refCounted)
		{
			T* refCountedCast = dynamic_cast<T*>(refCounted);
			assert(refCountedCast);
			if (refCountedCast)
				this->Set(refCounted);
		}

	private:
		ReferenceCounted* refCounted;
	};
}