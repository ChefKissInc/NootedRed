// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>
#include <IOKit/IOTypes.h>

class NRedAttributes {
    static constexpr UInt16 IsCatalina = (1U << 0);
    static constexpr UInt16 IsBigSurAndLater = (1U << 1);
    static constexpr UInt16 IsMonterey = (1U << 2);
    static constexpr UInt16 IsMontereyAndLater = (1U << 3);
    static constexpr UInt16 IsVentura = (1U << 4);
    static constexpr UInt16 IsVenturaAndLater = (1U << 5);
    static constexpr UInt16 IsVentura1304Based = (1U << 6);
    static constexpr UInt16 IsVentura1304AndLater = (1U << 7);
    static constexpr UInt16 IsSonoma1404AndLater = (1U << 8);
    static constexpr UInt16 IsRaven = (1U << 9);
    static constexpr UInt16 IsPicasso = (1U << 10);
    static constexpr UInt16 IsRaven2 = (1U << 11);
    static constexpr UInt16 IsRenoir = (1U << 12);
    static constexpr UInt16 IsRenoirE = (1U << 13);
    static constexpr UInt16 IsGreenSardine = (1U << 14);

    UInt16 value {0};

    public:
    inline const bool isCatalina() const { return (this->value & IsCatalina) != 0; }
    inline const bool isBigSurAndLater() const { return (this->value & IsBigSurAndLater) != 0; }
    inline const bool isMonterey() const { return (this->value & IsMonterey) != 0; }
    inline const bool isMontereyAndLater() const { return (this->value & IsMontereyAndLater) != 0; }
    inline const bool isVentura() const { return (this->value & IsVentura) != 0; }
    inline const bool isVenturaAndLater() const { return (this->value & IsVenturaAndLater) != 0; }
    inline const bool isVentura1304Based() const { return (this->value & IsVentura1304Based) != 0; }
    inline const bool isVentura1304AndLater() const { return (this->value & IsVentura1304AndLater) != 0; }
    inline const bool isSonoma1404AndLater() const { return (this->value & IsSonoma1404AndLater) != 0; }
    inline const bool isRaven() const { return (this->value & IsRaven) != 0; }
    inline const bool isPicasso() const { return (this->value & IsPicasso) != 0; }
    inline const bool isRaven2() const { return (this->value & IsRaven2) != 0; }
    inline const bool isRenoir() const { return (this->value & IsRenoir) != 0; }
    inline const bool isRenoirE() const { return (this->value & IsRenoirE) != 0; }
    inline const bool isGreenSardine() const { return (this->value & IsGreenSardine) != 0; }

    inline void setCatalina() { this->value |= IsCatalina; }
    inline void setBigSurAndLater() { this->value |= IsBigSurAndLater; }
    inline void setMonterey() { this->value |= IsMonterey; }
    inline void setMontereyAndLater() { this->value |= IsMontereyAndLater; }
    inline void setVentura() { this->value |= IsVentura; }
    inline void setVenturaAndLater() { this->value |= IsVenturaAndLater; }
    inline void setVentura1304Based() { this->value |= IsVentura1304Based; }
    inline void setVentura1304AndLater() { this->value |= IsVentura1304AndLater; }
    inline void setSonoma1404AndLater() { this->value |= IsSonoma1404AndLater; }
    inline void setRaven() { this->value |= IsRaven; }
    inline void setPicasso() { this->value |= IsPicasso; }
    inline void setRaven2() { this->value |= IsRaven2; }
    inline void setRenoir() { this->value |= IsRenoir; }
    inline void setRenoirE() { this->value |= IsRenoirE; }
    inline void setGreenSardine() { this->value |= IsGreenSardine; }

    inline const char *getChipName() const {
        if (this->isRaven2()) {
            return "raven2";
        } else if (this->isPicasso()) {
            return "picasso";
        } else if (this->isRaven()) {
            return "raven";
        } else if (this->isGreenSardine()) {
            return "green_sardine";
        } else if (this->isRenoir()) {
            return "renoir";
        } else {
            PANIC("NRed", "Internal error: Device is unknown");
        }
    }

    inline const char *getGCPrefix() const {
        if (this->isRaven2()) {
            return "gc_9_2_";
        } else if (this->isRaven()) {
            return "gc_9_1_";
        } else if (this->isRenoir()) {
            return "gc_9_3_";
        } else {
            PANIC("NRed", "Internal error: Device is unknown");
        }
    }
};
