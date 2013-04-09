/*  Copyright (C) 2013 VATSIM Community
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PQANGLE_H
#define PQANGLE_H
#include "blackmisc/pqphysicalquantity.h"

namespace BlackMisc
{
namespace PhysicalQuantities
{

/*!
 * \brief Physical unit degree
 * \author KWB
 */
class CAngle : public CPhysicalQuantity<CAngleUnit, CAngle>
{

public:
    /*!
     * \brief Default constructor
     */
    CAngle() : CPhysicalQuantity(0, CAngleUnit::rad(), CAngleUnit::rad()) {}
    /**
       * \brief Copy constructor
       */
    CAngle(const CAngle &angle) : CPhysicalQuantity(angle) {}
    /*!
     * \brief Init by int value
     * \param value
     * \param unit
     */
    CAngle(qint32 value, const CAngleUnit &unit): CPhysicalQuantity(value, unit, CAngleUnit::rad()) {}
    /*!
     * \brief Init by double value
     * \param value
     * \param unit
     */
    CAngle(double value, const CAngleUnit &unit): CPhysicalQuantity(value, unit, CAngleUnit::rad()) {}
    /*!
     * \brief Virtual destructor
     */
    virtual ~CAngle() {}
    /*!
     * \brief Convenience method PI
     * \return
     */
    const static double pi() {
        return M_PI;
    }
    /*!
     * \brief Value as factor of PI (e.g.0.5PI)
     * \return
     */
    double piFactor() const {
        return CMeasurementUnit::round(this->convertedSiValueToDouble() / M_PI, 6);
    }
};

} // namespace
} // namespace

#endif // PQANGLE_H
