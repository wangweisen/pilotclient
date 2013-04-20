/*  Copyright (C) 2013 VATSIM Community / contributors
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BLACKMISCTEST_SAMPLESGEO2ECEF_H
#define BLACKMISCTEST_SAMPLESGEO2ECEF_H
#include "blackmisc/coordinatetransformation.h"

namespace BlackMiscTest
{

/*!
 * \brief Samples for vector / matrix
 */
class CSamplesGeodeticToEcef
{
public:
    /*!
     * \brief Run the samples
     */
    static int samples();

private:
    /*!
     * \brief Avoid init
     */
    CSamplesGeodeticToEcef() {}
};
} // namespace

#endif // guard
