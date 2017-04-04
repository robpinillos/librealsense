// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
// Definition of Microsoft Extensions to USB Video Class 1.5
// supported by RealSense devices, namely RS4xx models.
// MS extension to UVC 1.5 enables new functionalities as well
// as enhancement to the established ones:
// Video metadata -  Intrinsic/Extrinsic/Face Authentication (new)
// Video controls - Focus/EV Compensation (new) Exposure/White Balance (modified)
// Latest spec version : MS Extensions for UVC 1.5  v.167

#pragma once

#include "option.h"
namespace rsimpl2
{
    // Depth subdevice XUs [depth:0;    vfID_UnitMsExtDepth = 0xE,  Node=3]
    const uvc::extension_unit ms_ctrl_depth_xu = { 0, 14, 3,
    { 0xf3f95dc, 0x2632, 0x4c4e,{ 0x92, 0xc9, 0xa0, 0x47, 0x82, 0xf4, 0x3b, 0xc8 } } };

    // FishEye subdevice XUs [fisheyeh:3; vfID_UnitMsExtFishEye  = 0xF, Node=TBD]
    const uvc::extension_unit ms_ctrl_fisheye_xu = { 3, 15, 4,
    { 0xf3f95dc, 0x2632, 0x4c4e,{ 0x92, 0xc9, 0xa0, 0x47, 0x82, 0xf4, 0x3b, 0xc8 } } };

    // MSXU Indentifiers
    enum msxu_ctrl : uint8_t
    {
        MSXU_FOCUS                  = 0x1,
        MSXU_EXPOSURE               = 0x2,
        MSXU_EVCOMPENSATION         = 0x3,
        MSXU_WHITEBALANCE           = 0x4,
        MSXU_RESERVED               = 0x5,
        MSXU_FACE_AUTHENTICATION    = 0x6,
        MSXU_CAMERA_EXTRINSIC       = 0x7,
        MSXU_CAMERA_INTRINSIC       = 0x8,
        MSXU_METADATA               = 0x9,
    };

    /** \brief msxu_description - MS XU details necessary for protocol implementation */
    struct msxu_description
    {
        uint16_t        _lenght;
        std::string     _desc;
    };

    /** \brief msxu_map - lists of MS XU controls and their actual properties */
    static const std::map< msxu_ctrl, msxu_description> msxu_map = {
        { msxu_ctrl::MSXU_FOCUS,              { 12, "Focus MS XU Control" } },
        { msxu_ctrl::MSXU_EXPOSURE,           { 15, "Exposure MS XU Control" } },
        { msxu_ctrl::MSXU_EVCOMPENSATION,     { 11, "EV Compensation MS XU Control" } },
        { msxu_ctrl::MSXU_WHITEBALANCE,       { 15, "White Balance MS XU Control" } },
        // The size the following is Vendor-specific. TBD
        { msxu_ctrl::MSXU_FACE_AUTHENTICATION,{ 1024, "Face Authentication MS XU Control" } },
        { msxu_ctrl::MSXU_CAMERA_EXTRINSIC,   { 1024, "Sensor Extrinsic MS XU Control" } },
        { msxu_ctrl::MSXU_CAMERA_INTRINSIC,   { 1024, "Sensor Intrinsic MS XU Control" } },
        { msxu_ctrl::MSXU_METADATA,           { 1024, "Metadata MS XU Control" } },
    };

    /** \brief msxu_ctrl_mode - MS XU control state (auto/manual/lock/...) is defined by 56 bit mask.
        D0 corresponds to bit:0, D1 to bit:1 and so on */
    enum msxu_ctrl_mode : uint8_t
    {
        MSXU_MODE_D0_AUTO       = 0x1,
        MSXU_MODE_D1_MANUAL     = 0x2,
        MSXU_MODE_D2_LOCK       = 0x4,
    };

    /** \brief msxu_fields - MS XU control layout. Applicable for AutoExposure and WhiteBalance */
    enum msxu_fields : uint8_t
    {
        msxu_mode           = 0,
        msxu_value          = 7,
    };

    /** \brief ms_xu_option is class template for Microsoft-defined XU Extensions. */
    template<typename T>
    class ms_xu_option : public uvc_xu_option<T>
    {
    public:
        // Required by GCC to resolve template parameters in base class
        using uvc_xu_option<T>::_ep;
        using uvc_xu_option<T>::_xu;
        using uvc_xu_option<T>::_id;

        void set(float value) override
        {
            _ep.invoke_powered(
                [this, value](uvc::uvc_device& dev)
            {
                std::vector<uint8_t>    _transmit_buf(_xu_length, 0);
                this->encode_data(value, _transmit_buf);
                if (!dev.set_xu(_xu, _id, const_cast<uint8_t*>(_transmit_buf.data()), _xu_length))
                    throw invalid_value_exception(to_string() << "set_xu(id=" << std::to_string(_id) << ") failed!" << " Last Error: " << strerror(errno));
            });
        }

        float query() const override
        {
            return static_cast<float>(_ep.invoke_powered(
                [this](uvc::uvc_device& dev)
            {
                std::vector<uint8_t>    _transmit_buf(_xu_length, 0);
                if (!dev.get_xu(_xu, _id, const_cast<uint8_t*>(_transmit_buf.data()), _xu_length))
                    throw invalid_value_exception(to_string() << "get_xu(id=" << std::to_string(_id) << ") failed!" << " Last Error: " << strerror(errno));

                return this->decode_data(_transmit_buf);
            }));
        }

        explicit ms_xu_option(uvc_endpoint& ep, const uvc::extension_unit& xu, msxu_ctrl ctrl_id, float def_value)
            : uvc_xu_option<T>(ep, xu, ctrl_id, msxu_map.at(ctrl_id)._desc),
            _xu_length(msxu_map.at(ctrl_id)._lenght), _def_value(def_value) {}

    protected:
        virtual void encode_data(const float& val, std::vector<uint8_t>& buf) const = 0; /**< Template method to be defined in derived classes*/
        virtual float decode_data(const std::vector<uint8_t>& buf) const = 0;

        const uint16_t _xu_length;       /**< The lenght of the data buffer to be exchanged with device*/
        float _def_value;
    };

    /** \brief ms_xu_control_option class provided template specialization
     * to control MS XU modes (control channel). */
    class ms_xu_control_option : public ms_xu_option<uint8_t>
    {
    public:
        const char* get_value_description(float val) const override
        {
            switch (static_cast<int>(val))
            {
            case 1:
            {
                return "Auto";
            }
            case 0:
            {
                return "Manual";
            }
            default:
                throw invalid_value_exception(msxu_map.at((msxu_ctrl)_id)._desc + " get_value_description() for value = "
                                               + std::to_string(val) + " is not supported");
            }
        }

        option_range get_range() const override;

        virtual void encode_data(const float& val, std::vector<uint8_t>& buf) const override;
        virtual float decode_data(const std::vector<uint8_t>& buf) const override;

        explicit ms_xu_control_option(uvc_endpoint& ep,
                                      const uvc::extension_unit& xu,
                                      msxu_ctrl ctrl_id,
                                      float def_value)
            : ms_xu_option(ep, xu, ctrl_id, def_value)
        {}

    private:
        void parse_data(std::vector<uint8_t>& data) const;

    };


    /** \brief ms_xu_control_option class provided template specialization to set/get MS XU values (data channel). */
    class ms_xu_data_option : public ms_xu_option<float>
    {
    public:
        option_range get_range() const override { return _range; }

        virtual void encode_data(const float& val, std::vector<uint8_t>& buf) const override;
        virtual float decode_data(const std::vector<uint8_t>& buf) const override;

        explicit ms_xu_data_option(uvc_endpoint& ep,
                                   const uvc::extension_unit& xu,
                                   msxu_ctrl ctrl_id,
                                   const option_range& range, float def_value)
            : ms_xu_option(ep, xu, ctrl_id, def_value), _range(range)
        {}

    private:
        option_range _range;
    };





    /** \brief auto_disabling_control class provided a control
     * that disable auto exposure whan changing the auto disabling control value */
    class auto_disabling_control : public option
    {
    public:
        const char* get_value_description(float val) const override
        {
            return _auto_disabling_control->get_value_description(val);
        }
        const char* get_description() const override
        {
             return _auto_disabling_control->get_description();
        }
        void set(float value) override
        {
           auto strong = _auto_exposure.lock();
           assert(strong);

           auto is_auto = strong->query();

           if(strong && is_auto)
           {
               LOG_DEBUG("Move auto exposure to manual mode in order set value to gain controll");
               strong->set(0);
           }
           _auto_disabling_control->set(value);
        }

        float query() const override
        {
            return _auto_disabling_control->query();
        }

        option_range get_range() const override
        {
            return _auto_disabling_control->get_range();
        }

        bool is_enabled() const override
        {
            return  _auto_disabling_control->is_enabled();
        }

        bool is_read_only() const override
        {
            return  _auto_disabling_control->is_read_only();
        }


        explicit auto_disabling_control(std::shared_ptr<option> auto_disabling,
                                        std::shared_ptr<option> auto_exposure)

            :_auto_disabling_control(auto_disabling), _auto_exposure(auto_exposure)
        {}

    private:
        std::shared_ptr<option> _auto_disabling_control;
        std::weak_ptr<option> _auto_exposure;

    };
}
