#pragma once

#include "Defines.h"

namespace Collision
{
	/**
	 * This class just contains some static methods that are useful when
	 * dealing with angles (in radians.)
	 */
	class COLLISION_LIB_API Angle
	{
	public:
		/**
		 * Move the first angle as close to the second in multiples of 2*pi.
		 *
		 * @param[in,out] angleA A multiple of 2*pi is added to this angle.
		 * @param[in] angleB This is the target angle.
		 */
		static void MakeClose(double& angleA, double angleB);
	};
}