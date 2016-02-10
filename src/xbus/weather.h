/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#ifndef BLACKSIM_XBUS_WEATHER_H
#define BLACKSIM_XBUS_WEATHER_H

//! \file

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "datarefs.h"
#include <QObject>

//! \cond PRIVATE
#define XBUS_WEATHER_INTERFACENAME "org.swift_project.xbus.weather"
#define XBUS_WEATHER_OBJECTPATH "/xbus/weather"
//! \endcond

namespace XBus
{

    /*!
     * XBus weather object which is accessible through DBus
     */
    class CWeather : public QObject
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", XBUS_WEATHER_INTERFACENAME)

    public:
        //! Constructor
        CWeather(QObject *parent) : QObject(parent) {}

        //! DBus interface name
        static const QString &InterfaceName()
        {
            static QString s(XBUS_WEATHER_INTERFACENAME);
            return s;
        }

        //! DBus object path
        static const QString &ObjectPath()
        {
            static QString s(XBUS_WEATHER_OBJECTPATH);
            return s;
        }

    public slots:
        //! True if the sim is using X-Plane built-in real weather source.
        bool isUsingRealWeather() const { return m_useRealWeather.get(); }

        //! Set whether or not to use X-Plane built-in real weather source.
        void setUseRealWeather(bool enable) { m_useRealWeather.set(enable ? 1 : 0); }

        //! Set reported visibility in meters.
        void setVisibility(float visibilityM) { m_visibilityM.set(visibilityM); }

        //! Set temperature at sea level in degrees C.
        void setTemperature(int degreesC) { m_temperatureC.set(degreesC); }

        //! Set dew point at sea level in degrees C.
        void setDewPoint(int degreesC) { m_dewPointC.set(degreesC); }

        //! Set barometric pressure at sea level in inches of mercury.
        void setQNH(float inHg) { m_qnhInhg.set(inHg); }

        //! Set amount of precipitation between 0 and 1.
        void setPrecipitationRatio(float precipRatio) { m_precipRatio.set(precipRatio); }

        //! Set amount of thunderstorms between 0 and 1.
        void setThunderstormRatio(float cbRatio) { m_cbRatio.set(cbRatio); }

        //! Set amount of turbulence between 0 and 1.
        void setTurbulenceRatio(float turbulenceRatio) { m_turbulenceRatio.set(turbulenceRatio); }

        //! Set runway friction, 0=dry, 1=damp, 2=wet.
        void setRunwayFriction(int friction) { m_runwayFriction.set(friction); }

        //! Set a cloud layer.
        //! \param layer Layer 0, 1, or 2.
        //! \param base Cloud base in meters above mean sea level.
        //! \param tops Cloud tops in meters above mean sea level.
        //! \param type Type of cloud: 0=clear, 1=cirrus, 2=scattered, 3=broken, 4=overcast, 5=stratus.
        //! \param coverage Amount of sky covered [0,6].
        void setCloudLayer(int layer, int base, int tops, int type, int coverage);

        //! Set a wind layer.
        //! \param layer Layer 0, 1, or 2.
        //! \param altitude Altitude in middle of layer in meters above mean sea level.
        //! \param direction Direction from which wind is blowing in degrees true.
        //! \param speed Wind speed in knots.
        //! \param shearDirection Direction from which wind shears blow in degrees true.
        //! \param shearSpeed Wind speed gain in knots (e.g. speed=10 and shearSpeed=5 means speed varies between 10 and 15).
        //! \param turbulence Amount of turbulence [0,10].
        void setWindLayer(int layer, int altitude, float direction, int speed, int shearDirection, int shearSpeed, int turbulence);

    private:
        DataRef<xplane::data::sim::weather::use_real_weather_bool> m_useRealWeather;
        DataRef<xplane::data::sim::weather::visibility_reported_m> m_visibilityM;
        DataRef<xplane::data::sim::weather::rain_percent> m_precipRatio;
        DataRef<xplane::data::sim::weather::thunderstorm_percent> m_cbRatio;
        DataRef<xplane::data::sim::weather::wind_turbulence_percent> m_turbulenceRatio;
        DataRef<xplane::data::sim::weather::temperature_sealevel_c> m_temperatureC;
        DataRef<xplane::data::sim::weather::dewpoi_sealevel_c> m_dewPointC;
        DataRef<xplane::data::sim::weather::barometer_sealevel_inhg> m_qnhInhg;
        DataRef<xplane::data::sim::weather::runway_friction> m_runwayFriction;
        struct
        {
            DataRef<xplane::data::sim::weather::cloud_base_msl_m_0> base;
            DataRef<xplane::data::sim::weather::cloud_tops_msl_m_0> tops;
            DataRef<xplane::data::sim::weather::cloud_type_0> type;
            DataRef<xplane::data::sim::weather::cloud_coverage_0> coverage;
        } m_cloudLayer0;
        struct
        {
            DataRef<xplane::data::sim::weather::cloud_base_msl_m_1> base;
            DataRef<xplane::data::sim::weather::cloud_tops_msl_m_1> tops;
            DataRef<xplane::data::sim::weather::cloud_type_1> type;
            DataRef<xplane::data::sim::weather::cloud_coverage_1> coverage;
        } m_cloudLayer1;
        struct
        {
            DataRef<xplane::data::sim::weather::cloud_base_msl_m_2> base;
            DataRef<xplane::data::sim::weather::cloud_tops_msl_m_2> tops;
            DataRef<xplane::data::sim::weather::cloud_type_2> type;
            DataRef<xplane::data::sim::weather::cloud_coverage_2> coverage;
        } m_cloudLayer2;
        struct
        {
            DataRef<xplane::data::sim::weather::wind_altitude_msl_m_0> altitude;
            DataRef<xplane::data::sim::weather::wind_direction_degt_0> direction;
            DataRef<xplane::data::sim::weather::wind_speed_kt_0> speed;
            DataRef<xplane::data::sim::weather::shear_direction_degt_0> shearDirection;
            DataRef<xplane::data::sim::weather::shear_speed_kt_0> shearSpeed;
            DataRef<xplane::data::sim::weather::turbulence_0> turbulence;
        } m_windLayer0;
        struct
        {
            DataRef<xplane::data::sim::weather::wind_altitude_msl_m_1> altitude;
            DataRef<xplane::data::sim::weather::wind_direction_degt_1> direction;
            DataRef<xplane::data::sim::weather::wind_speed_kt_1> speed;
            DataRef<xplane::data::sim::weather::shear_direction_degt_1> shearDirection;
            DataRef<xplane::data::sim::weather::shear_speed_kt_1> shearSpeed;
            DataRef<xplane::data::sim::weather::turbulence_1> turbulence;
        } m_windLayer1;
        struct
        {
            DataRef<xplane::data::sim::weather::wind_altitude_msl_m_2> altitude;
            DataRef<xplane::data::sim::weather::wind_direction_degt_2> direction;
            DataRef<xplane::data::sim::weather::wind_speed_kt_2> speed;
            DataRef<xplane::data::sim::weather::shear_direction_degt_2> shearDirection;
            DataRef<xplane::data::sim::weather::shear_speed_kt_2> shearSpeed;
            DataRef<xplane::data::sim::weather::turbulence_2> turbulence;
        } m_windLayer2;
    };

}

#endif // guard

