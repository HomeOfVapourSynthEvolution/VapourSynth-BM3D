/*
* BM3D denoising filter - VapourSynth plugin
* Copyright (C) 2015  mawen1250
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef TYPE_H_
#define TYPE_H_


#include <cstdint>
#include <cfloat>


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef int8_t sint8;
typedef int16_t sint16;
typedef int32_t sint32;
typedef int64_t sint64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef long double ldbl;


typedef int PCType;
typedef float FLType;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// determine whether _Ty satisfies Signed Int requirements
template<typename _Ty>
struct _IsSInt : std::integral_constant<bool,
    std::is_same<_Ty, signed char>::value ||
    std::is_same<_Ty, short>::value ||
    std::is_same<_Ty, int>::value ||
    std::is_same<_Ty, long>::value ||
    std::is_same<_Ty, long long>::value>
{};

// determine whether _Ty satisfies Unsigned Int requirements
template<typename _Ty>
struct _IsUInt : std::integral_constant<bool,
    std::is_same<_Ty, unsigned char>::value ||
    std::is_same<_Ty, unsigned short>::value ||
    std::is_same<_Ty, unsigned int>::value ||
    std::is_same<_Ty, unsigned long>::value ||
    std::is_same<_Ty, unsigned long long>::value>
{};

// determine whether _Ty satisfies Int requirements
template<typename _Ty>
struct _IsInt : std::integral_constant<bool,
    _IsSInt<_Ty>::value ||
    _IsUInt<_Ty>::value>
{};

// determine whether _Ty satisfies Float requirements
template<typename _Ty>
struct _IsFloat : std::integral_constant<bool,
    std::is_floating_point<_Ty>::value>
{};


#define isSInt(T) (_IsSInt<T>::value)
#define isUInt(T) (_IsUInt<T>::value)
#define isInt(T) (_IsInt<T>::value)
#define isFloat(T) (_IsFloat<T>::value)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Min value and Max value of each numeric type
#define IntMin(type) (type(sizeof(type) <= 1 ? INT8_MIN : sizeof(type) <= 2 ? INT16_MIN : sizeof(type) <= 4 ? INT32_MIN : INT64_MIN))
#define IntMax(type) (type(sizeof(type) <= 1 ? INT8_MAX : sizeof(type) <= 2 ? INT16_MAX : sizeof(type) <= 4 ? INT32_MAX : INT64_MAX))
#define UIntMin(type) (type(0))
#define UIntMax(type) (type(sizeof(type) <= 1 ? UINT8_MAX : sizeof(type) <= 2 ? UINT16_MAX : sizeof(type) <= 4 ? UINT32_MAX : UINT64_MAX))
#define FltMin(type) (type(sizeof(type) <= 4 ? FLT_MIN : sizeof(type) <= 8 ? DBL_MIN : LDBL_MIN))
#define FltMax(type) (type(sizeof(type) <= 4 ? FLT_MAX : sizeof(type) <= 8 ? DBL_MAX : LDBL_MAX))
#define FltNegMax(type) (type(sizeof(type) <= 4 ? -FLT_MAX : sizeof(type) <= 8 ? -DBL_MAX : -LDBL_MAX))


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Ty >
_Ty _TypeMinInt(const std::false_type &)
{
    return UIntMin(_Ty);
}

template < typename _Ty >
_Ty _TypeMinInt(const std::true_type &)
{
    return IntMin(_Ty);
}

template < typename _Ty >
_Ty _TypeMinFloat(const std::false_type &)
{
    return _TypeMinInt<_Ty>(_IsSInt<_Ty>());
}

template < typename _Ty >
_Ty _TypeMinFloat(const std::true_type &)
{
    return FltNegMax(_Ty);
}

template < typename _Ty >
_Ty TypeMin()
{
    return _TypeMinFloat<_Ty>(_IsFloat<_Ty>());
}

template < typename _Ty >
_Ty TypeMin(const _Ty &)
{
    return TypeMin<_Ty>();
}


template < typename _Ty >
_Ty _TypeMaxInt(const std::false_type &)
{
    return UIntMax(_Ty);
}

template < typename _Ty >
_Ty _TypeMaxInt(const std::true_type &)
{
    return IntMax(_Ty);
}

template < typename _Ty >
_Ty _TypeMaxFloat(const std::false_type &)
{
    return _TypeMaxInt<_Ty>(_IsSInt<_Ty>());
}

template < typename _Ty >
_Ty _TypeMaxFloat(const std::true_type &)
{
    return FltMax(_Ty);
}

template < typename _Ty >
_Ty TypeMax()
{
    return _TypeMaxFloat<_Ty>(_IsFloat<_Ty>());
}

template < typename _Ty >
_Ty TypeMax(const _Ty &)
{
    return TypeMax<_Ty>();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct Pos;
struct Pos3;


struct Pos
{
    typedef Pos _Myt;

    PCType y = 0;
    PCType x = 0;

    explicit Pos(PCType _y = 0, PCType _x = 0)
        : y(_y), x(_x)
    {}

    explicit Pos(Pos3 &_pos3);

    _Myt &operator=(const Pos3 &right);

    bool operator==(const _Myt &right) const
    {
        return y == right.y && x == right.x;
    }

    bool operator!=(const _Myt &right) const
    {
        return y != right.y || x != right.x;
    }

    bool operator<(const _Myt &right) const
    {
        if (y < right.y)
        {
            return true;
        }
        else if (y > right.y)
        {
            return false;
        }
        else if (x < right.x)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool operator>(const _Myt &right) const
    {
        if (y > right.y)
        {
            return true;
        }
        else if (y < right.y)
        {
            return false;
        }
        else if (x > right.x)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool operator>=(const _Myt &right) const
    {
        return !(*this < right);
    }

    bool operator<=(const _Myt &right) const
    {
        return !(*this > right);
    }

    friend std::ostream &operator<<(std::ostream &out, const _Myt &src)
    {
        out << "(" << src.y << ", " << src.x << ")";

        return out;
    }
};


struct Pos3
{
    typedef Pos3 _Myt;

    PCType z = 0;
    PCType y = 0;
    PCType x = 0;

    explicit Pos3(PCType _z = 0, PCType _y = 0, PCType _x = 0)
        : z(_z), y(_y), x(_x)
    {}

    explicit Pos3(Pos &_pos2, PCType _z = 0);

    _Myt &operator=(const Pos &right);

    bool operator==(const _Myt &right) const
    {
        return z == right.z && y == right.y && x == right.x;
    }

    bool operator!=(const _Myt &right) const
    {
        return z != right.z || y != right.y || x != right.x;
    }

    bool operator<(const _Myt &right) const
    {
        if (z < right.z)
        {
            return true;
        }
        else if (z > right.z)
        {
            return false;
        }
        else if (y < right.y)
        {
            return true;
        }
        else if (y > right.y)
        {
            return false;
        }
        else if (x < right.x)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool operator>(const _Myt &right) const
    {
        if (z > right.z)
        {
            return true;
        }
        else if (z < right.z)
        {
            return false;
        }
        else if (y > right.y)
        {
            return true;
        }
        else if (y < right.y)
        {
            return false;
        }
        else if (x > right.x)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool operator>=(const _Myt &right) const
    {
        return !(*this < right);
    }

    bool operator<=(const _Myt &right) const
    {
        return !(*this > right);
    }

    friend std::ostream &operator<<(std::ostream &out, const _Myt &src)
    {
        out << "(" << src.z << ", " << src.y << ", " << src.x << ")";

        return out;
    }
};


inline Pos::Pos(Pos3 &_pos3)
    : y(_pos3.y), x(_pos3.x)
{}

inline Pos &Pos::operator=(const Pos3 &right)
{
    y = right.y;
    x = right.x;
    return *this;
}


inline Pos3::Pos3(Pos &_pos2, PCType _z)
    : z(_z), y(_pos2.y), x(_pos2.x)
{}

inline Pos3 &Pos3::operator=(const Pos &right)
{
    y = right.y;
    x = right.x;
    return *this;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Ty1, typename _Ty2 >
struct KeyPair
    : public std::pair<_Ty1, _Ty2>
{
    typedef KeyPair<_Ty1, _Ty2> _Myt;
    typedef std::pair<_Ty1, _Ty2> _Mybase;

    typedef _Ty1 KeyType;
    typedef _Ty2 ValType;

    KeyPair()
        : _Mybase()
    {}

    KeyPair(const _Ty1& _Val1, const _Ty2& _Val2)
        : _Mybase(_Val1, _Val2)
    {}

    KeyPair(const _Myt &_Right)
        : _Mybase(_Right)
    {}

    KeyPair(_Myt &&_Right)
        : _Mybase(_Right)
    {}

    _Myt &operator=(const _Myt &_Right)
    {
        _Mybase::operator=(_Right);
        return *this;
    }

    _Myt &operator=(_Myt &&_Right)
    {
        _Mybase::operator=(_Right);
        return *this;
    }

    bool operator==(const _Myt &_Right)
    {
        return this->first == _Right.first;
    }

    bool operator!=(const _Myt &_Right)
    {
        return this->first != _Right.first;
    }

    bool operator<(const _Myt &_Right)
    {
        return this->first < _Right.first;
    }

    bool operator>(const _Myt &_Right)
    {
        return this->first > _Right.first;
    }

    bool operator<=(const _Myt &_Right)
    {
        return this->first <= _Right.first;
    }

    bool operator>=(const _Myt &_Right)
    {
        return this->first >= _Right.first;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
