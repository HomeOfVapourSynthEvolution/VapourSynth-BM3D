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


#ifndef BLOCK_H_
#define BLOCK_H_

#include <iostream>
#include <vector>
#include <algorithm>
#include "Type.h"
#include "emmintrin.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Ty = double,
    typename _DTy = double >
class Block
{
public:
    typedef Block<_Ty, _DTy> _Myt;
    typedef _Ty value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef _Ty *pointer;
    typedef const _Ty *const_pointer;
    typedef _Ty &reference;
    typedef const _Ty &const_reference;

    typedef pointer iterator;
    typedef const_pointer const_iterator;

    typedef _DTy dist_type;

    typedef dist_type KeyType;
    typedef Pos PosType;
    typedef KeyPair<KeyType, PosType> PosPair;
    typedef std::vector<KeyType> KeyCode;
    typedef std::vector<PosType> PosCode;
    typedef std::vector<PosPair> PosPairCode;

private:
    PCType Height_ = 0;
    PCType Width_ = 0;
    PCType PixelCount_ = 0;
    PosType pos_;
    pointer Data_ = nullptr;

public:
    template < typename _Fn1 >
    void for_each(_Fn1 _Func)
    {
        Block_For_each(*this, _Func);
    }

    template < typename _Fn1 >
    void for_each(_Fn1 _Func) const
    {
        Block_For_each(*this, _Func);
    }

    template < typename _St2, typename _Fn1 >
    void for_each(_St2 &data2, _Fn1 _Func)
    {
        Block_For_each(*this, data2, _Func);
    }

    template < typename _St2, typename _Fn1 >
    void for_each(_St2 &data2, _Fn1 _Func) const
    {
        Block_For_each(*this, data2, _Func);
    }

    template < typename _Fn1 >
    void transform(_Fn1 _Func)
    {
        Block_Transform(*this, _Func);
    }

    template < typename _St1, typename _Fn1 >
    void transform(const _St1 &src, _Fn1 _Func)
    {
        Block_Transform(*this, src, _Func);
    }

public:
    // Default constructor
    Block() {}

    Block(PCType _Height, PCType _Width, const PosType &pos, bool Init = true, value_type Value = 0)
        : Height_(_Height), Width_(_Width), PixelCount_(Height_ * Width_), pos_(pos)
    {
        AlignedMalloc(Data_, size());

        InitValue(Init, Value);
    }

    // Constructor from plane pointer and PosType
    template < typename _St1 >
    Block(const _St1 *src, PCType src_stride, PCType _Height, PCType _Width, const PosType &pos)
        : Block(_Height, _Width, pos, false, 0)
    {
        From(src, src_stride);
    }

    // Constructor from src Block
    Block(const _Myt &src, bool Init, value_type Value = 0)
        : Block(src.Height_, src.Width_, src.pos_, Init, Value)
    {}

    // Copy constructor
    Block(const _Myt &src)
        : Block(src.Height_, src.Width_, src.pos_, false, 0)
    {
        memcpy(Data_, src.Data_, sizeof(value_type) * size());
    }

    // Move constructor
    Block(_Myt &&src)
        : Height_(src.Height_), Width_(src.Width_), PixelCount_(src.PixelCount_), pos_(src.pos_)
    {
        Data_ = src.Data_;

        src.Height_ = 0;
        src.Width_ = 0;
        src.PixelCount_ = 0;
        src.Data_ = nullptr;
    }

    // Destructor
    ~Block()
    {
        AlignedFree(Data_);
    }

    // Copy assignment operator
    _Myt &operator=(const _Myt &src)
    {
        if (this == &src)
        {
            return *this;
        }

        Height_ = src.Height_;
        Width_ = src.Width_;
        PixelCount_ = src.PixelCount_;
        pos_ = src.pos_;

        memcpy(Data_, src.Data_, sizeof(value_type) * size());

        return *this;
    }

    // Move assignment operator
    _Myt &operator=(_Myt &&src)
    {
        if (this == &src)
        {
            return *this;
        }

        Height_ = src.Height_;
        Width_ = src.Width_;
        PixelCount_ = src.PixelCount_;
        pos_ = src.pos_;

        AlignedFree(Data_);
        Data_ = src.Data_;

        src.Height_ = 0;
        src.Width_ = 0;
        src.PixelCount_ = 0;
        src.Data_ = nullptr;

        return *this;
    }

    reference operator[](PCType i) { return Data_[i]; }
    const_reference operator[](PCType i) const { return Data_[i]; }
    reference operator()(PCType j, PCType i) { return Data_[j * Stride() + i]; }
    const_reference operator()(PCType j, PCType i) const { return Data_[j * Stride() + i]; }

    iterator begin() { return Data_; }
    const_iterator begin() const { return Data_; }
    iterator end() { return Data_ + PixelCount_; }
    const_iterator end() const { return Data_ + PixelCount_; }
    size_type size() const { return PixelCount_; }
    pointer data() { return Data_; }
    const_pointer data() const { return Data_; }
    value_type value(PCType i) { return Data_[i]; }
    const value_type value(PCType i) const { return Data_[i]; }

    __forceinline PCType Height() const { return Height_; }
	__forceinline PCType Width() const { return Width_; }
	__forceinline PCType Stride() const { return Width_; }
	__forceinline PCType PixelCount() const { return PixelCount_; }
	__forceinline PosType GetPos() const { return pos_; }
	__forceinline PCType PosY() const { return pos_.y; }
	__forceinline PCType PosX() const { return pos_.x; }

    void SetPos(PosType _pos) { pos_ = _pos; }

    friend std::ostream &operator<<(std::ostream &out, const _Myt &src)
    {
        out << "Block Info\n"
            << "    PixelCount = " << src.PixelCount() << std::endl
            << "    Pos(y, x) = " << src.GetPos() << std::endl
            << "    Data[" << src.Height() << "][" << src.Width() << "] = ";

        auto srcp = src.data();

        for (PCType j = 0; j < src.Height(); ++j)
        {
            out << "\n        ";

            for (PCType i = 0; i < src.Width(); ++i, ++srcp)
            {
                out << *srcp << " ";
            }
        }

        out << std::endl;

        return out;
    }

    ////////////////////////////////////////////////////////////////
    // Initialization functions

    void InitValue(bool Init = true, value_type Value = 0)
    {
        if (Init)
        {
            for_each([&](value_type &x)
            {
                x = Value;
            });
        }
    }

    ////////////////////////////////////////////////////////////////
    // Read/Store functions

    template < typename _St1 >
    void From(const _St1 *src, PCType src_stride)
    {
        auto dstp = data();
        auto srcp = src + PosY() * src_stride + PosX();

        for (PCType y = 0; y < Height(); ++y)
        {
            PCType x = y * src_stride;

            for (PCType upper = x + Width(); x < upper; ++x, ++dstp)
            {
                *dstp = static_cast<value_type>(srcp[x]);
            }
        }
    }

    template < typename _St1 >
    void From(const _St1 *src, PCType src_stride, const PosType &pos)
    {
        pos_ = pos;
        From(src, src_stride);
    }

    template < typename _Dt1 >
    void To(_Dt1 *dst, PCType dst_stride) const
    {
        auto srcp = data();
        auto dstp = dst + PosY() * dst_stride + PosX();

        for (PCType y = 0; y < Height(); ++y)
        {
            PCType x = y * dst_stride;

            for (PCType upper = x + Width(); x < upper; ++x, ++srcp)
            {
                dstp[x] = static_cast<_Dt1>(*srcp);
            }
        }
    }

    ////////////////////////////////////////////////////////////////
    // Accumulate functions

    template < typename _St1 >
    void AddFrom(const _St1 *src, PCType src_stride, const PosType &pos)
    {
        auto dstp = data();
        auto srcp = src + pos.y * src_stride + pos.x;

        for (PCType y = 0; y < Height(); ++y)
        {
            PCType x = y * src_stride;

            for (PCType upper = x + Width(); x < upper; ++x, ++dstp)
            {
                *dstp += static_cast<value_type>(srcp[x]);
            }
        }
    }

    template < typename _St1, typename _Gt1 >
    void AddFrom(const _St1 *src, PCType src_stride, const PosType &pos, _Gt1 gain)
    {
        auto dstp = data();
        auto srcp = src + pos.y * src_stride + pos.x;

        for (PCType y = 0; y < Height(); ++y)
        {
            PCType x = y * src_stride;

            for (PCType upper = x + Width(); x < upper; ++x, ++dstp)
            {
                *dstp += static_cast<value_type>(srcp[x] * gain);
            }
        }
    }

    template < typename _Dt1 >
    void AddTo(_Dt1 *dst, PCType dst_stride) const
    {
        auto srcp = data();
        auto dstp = dst + PosY() * dst_stride + PosX();

        for (PCType y = 0; y < Height(); ++y)
        {
            PCType x = y * dst_stride;

            for (PCType upper = x + Width(); x < upper; ++x, ++srcp)
            {
                dstp[x] += static_cast<_Dt1>(*srcp);
            }
        }
    }

    template < typename _Dt1, typename _Gt1 >
    void AddTo(_Dt1 *dst, PCType dst_stride, _Gt1 gain) const
    {
        auto srcp = data();
        auto dstp = dst + PosY() * dst_stride + PosX();

        for (PCType y = 0; y < Height(); ++y)
        {
            PCType x = y * dst_stride;

            for (PCType upper = x + Width(); x < upper; ++x, ++srcp)
            {
                dstp[x] += static_cast<_Dt1>(*srcp * gain);
            }
        }
    }

    template < typename _Dt1 >
    void CountTo(_Dt1 *dst, PCType dst_stride) const
    {
        auto dstp = dst + PosY() * dst_stride + PosX();

        for (PCType y = 0; y < Height(); ++y)
        {
            PCType x = y * dst_stride;

            for (PCType upper = x + Width(); x < upper; ++x)
            {
                ++dstp[x];
            }
        }
    }

    template < typename _Dt1 >
    void CountTo(_Dt1 *dst, PCType dst_stride, _Dt1 value) const
    {
        auto dstp = dst + PosY() * dst_stride + PosX();

        for (PCType y = 0; y < Height(); ++y)
        {
            PCType x = y * dst_stride;

            for (PCType upper = x + Width(); x < upper; ++x)
            {
                dstp[x] += value;
            }
        }
    }

    ////////////////////////////////////////////////////////////////
    // Single block-matching functions

    template < typename _St1 >
    PosPair BlockMatching(const _St1 *src, PCType src_height, PCType src_width, PCType src_stride, _St1 src_range,
        PCType range, PCType step, double thMSE = 10, bool excludeCurPos = false) const
    {
        bool end = false;
        PosType pos;
        dist_type temp;
        dist_type distMin = TypeMax<dist_type>();

        range = range / step * step;
        const PCType l = SearchBoundary(PCType(0), range, step, false);
        const PCType r = SearchBoundary(src_width - Width(), range, step, false);
        const PCType t = SearchBoundary(PCType(0), range, step, true);
        const PCType b = SearchBoundary(src_height - Height(), range, step, true);

        double MSE2SSE = static_cast<double>(PixelCount()) * src_range * src_range / double(255 * 255);
        double distMul = double(1) / MSE2SSE;
        dist_type thSSE = static_cast<dist_type>(thMSE * MSE2SSE);

        for (PCType j = t; j <= b; j += step)
        {
            for (PCType i = l; i <= r; i += step)
            {
                dist_type dist = 0;

                if (excludeCurPos && j == PosY() && i == PosX())
                {
                    continue;
                }
                else
                {
                    auto refp = data();
                    auto srcp = src + j * src_stride + i;

                    for (PCType y = 0; y < Height(); ++y)
                    {
                        PCType x = y * src_stride;

                        for (PCType upper = x + Width(); x < upper; ++x, ++refp)
                        {
                            temp = static_cast<dist_type>(*refp) - static_cast<dist_type>(srcp[x]);
                            dist += temp * temp;
                        }
                    }
                }

                if (dist < distMin)
                {
                    distMin = dist;
                    pos.y = j;
                    pos.x = i;

                    if (distMin <= thSSE)
                    {
                        end = true;
                        break;
                    }
                }
            }

            if (end)
            {
                break;
            }
        }

        return PosPair(static_cast<KeyType>(distMin * distMul), pos);
    }

    ////////////////////////////////////////////////////////////////
    // Multiple block-matching functions

    template < typename _St1 >
    void BlockMatchingMulti(PosPairCode &match_code, const _St1 *src, PCType src_stride, _St1 src_range,
        const PosCode &search_pos, double thMSE) const
    {
        double MSE2SSE = static_cast<double>(PixelCount()) * src_range * src_range / double(255 * 255);
        double distMul = double(1) / MSE2SSE;
        dist_type thSSE = static_cast<dist_type>(thMSE * MSE2SSE);

        size_t index = match_code.size();
        match_code.resize(index + search_pos.size());

		PCType strbroder = Width() % 8;
        for (auto pos : search_pos)
        {
            dist_type dist = 0;

            auto refp = data();
            auto srcp = src + pos.y * src_stride + pos.x;
			__m128 ssum = _mm_setzero_ps();
            for (PCType y = 0; y < Height(); ++y)
            {
                PCType m = y * src_stride;
				PCType x = m;
                
				if (strbroder)
				{
					PCType upper = m + strbroder;
					do {
						dist_type temp = static_cast<dist_type>(*refp) - static_cast<dist_type>(srcp[x]);
						dist += temp * temp;
						++x, ++refp;
					} while (x < upper);
				}

				_mm_prefetch((const char*)(refp), _MM_HINT_NTA);
				_mm_prefetch((const char*)(refp + 4), _MM_HINT_NTA);
				_mm_prefetch((const char*)(&srcp[x]), _MM_HINT_NTA);
				_mm_prefetch((const char*)(&srcp[x + 4]), _MM_HINT_NTA);

				for (PCType upper = m + Width(); x < upper; x+=8 , refp+=8)
				{
					__m128 a = _mm_load_ps(const_cast<const float*>((float*)refp));
					__m128 c = _mm_load_ps(const_cast<const float*>((float*)(refp + 4)));
					__m128 b = _mm_load_ps(const_cast<const float*>((float*)&srcp[x]));
					__m128 d = _mm_load_ps(const_cast<const float*>((float*)&srcp[x + 4]));
					__m128 e = _mm_sub_ps(a, b);
					__m128 f = _mm_sub_ps(c, d);
					b = _mm_mul_ps(e, e);
					d = _mm_mul_ps(f, f);
					ssum = _mm_add_ps(ssum, b);
					ssum = _mm_add_ps(ssum, d);
				}
				
            }
			dist += ssum.m128_f32[0] + ssum.m128_f32[1] + ssum.m128_f32[2] + ssum.m128_f32[3];

            // Only match similar blocks but not identical blocks
            if (dist <= thSSE && dist != 0)
            {
                match_code[index++] = PosPair(static_cast<KeyType>(dist * distMul), pos);
            }
        }

        match_code.resize(index);
    }

    template < typename _St1 >
	__forceinline PosPairCode BlockMatchingMulti(const _St1 *src, PCType src_stride, _St1 src_range,
        const PosCode &search_pos, double thMSE, size_t match_size = 0, bool sorted = true) const
    {
        PosPairCode match_code;

        BlockMatchingMulti(match_code, src, src_stride, src_range, search_pos, thMSE);

        // When match_size > 0, it's the upper limit of the number of matched blocks
        if (match_size > 0 && match_code.size() > match_size)
        {
            // Always sorted when size of match code is larger than match_size,
            // since std::partial_sort is faster than std::nth_element
            std::partial_sort(match_code.begin(), match_code.begin() + match_size, match_code.end());
            match_code.resize(match_size);
        }
        else if (sorted)
        {
            std::stable_sort(match_code.begin(), match_code.end());
        }

        return match_code;
    }

    // excludeCurPos:
    //     0 - include current position in search positions
    //     1 - exclude current position in search positions but take it as the first element in matched code
    //     2 - exclude current position in search positions
    template < typename _St1 >
    PosPairCode BlockMatchingMulti(const _St1 *src, PCType src_height, PCType src_width, PCType src_stride, _St1 src_range,
        PCType range, PCType step, double thMSE, int excludeCurPos = 1, size_t match_size = 0, bool sorted = true) const
    {
        range = range / step * step;
        const PCType l = SearchBoundary(PCType(0), range, step, false);
        const PCType r = SearchBoundary(src_width - Width(), range, step, false);
        const PCType t = SearchBoundary(PCType(0), range, step, true);
        const PCType b = SearchBoundary(src_height - Height(), range, step, true);

        PosCode search_pos(((r - l) / step + 1) * ((b - t) / step + 1));
        size_t index = 0;

        for (PCType j = t; j <= b; j += step)
        {
            for (PCType i = l; i <= r; i += step)
            {
                if (excludeCurPos > 0 && j == PosY() && i == PosX())
                {
                    continue;
                }

                search_pos[index++] = PosType(j, i);
            }
        }

        PosPairCode match_code;
        if (excludeCurPos == 1) match_code.push_back(PosPair(static_cast<KeyType>(0), PosType(PosY(), PosX())));

        BlockMatchingMulti(match_code, src, src_stride, src_range, search_pos, thMSE);

        // When match_size > 0, it's the upper limit of the number of matched blocks
        if (match_size > 0 && match_code.size() > match_size)
        {
            // Always sorted when size of match code is larger than match_size,
            // since std::partial_sort is faster than std::nth_element
            std::partial_sort(match_code.begin(), match_code.begin() + match_size, match_code.end());
            match_code.resize(match_size);
        }
        else if (sorted)
        {
            std::stable_sort(match_code.begin(), match_code.end());
        }

        return match_code;
    }

    ////////////////////////////////////////////////////////////////
    // Search window helper functions

    static PCType _SearchBoundary(PCType pos, PCType plane_boundary, PCType search_range, PCType search_step)
    {
        PCType search_boundary;

        search_range = search_range / search_step * search_step;

        if (pos == plane_boundary)
        {
            search_boundary = plane_boundary;
        }
        else if (pos > plane_boundary)
        {
            search_boundary = pos - search_range;

            while (search_boundary < plane_boundary)
            {
                search_boundary += search_step;
            }
        }
        else
        {
            search_boundary = pos + search_range;

            while (search_boundary > plane_boundary)
            {
                search_boundary -= search_step;
            }
        }

        return search_boundary;
    }

    PCType SearchBoundary(PCType plane_boundary, PCType search_range, PCType search_step, bool vertical) const
    {
        return _SearchBoundary(vertical ? PosY() : PosX(), plane_boundary, search_range, search_step);
    }

    void AddSearchPos(PosCode &search_pos, size_t &index, PosType ref_pos, PCType src_height, PCType src_width,
        PCType range, PCType step = 1) const
    {
        range = range / step * step;
        const PCType l = _SearchBoundary(ref_pos.x, PCType(0), range, step);
        const PCType r = _SearchBoundary(ref_pos.x, src_width - Width(), range, step);
        const PCType t = _SearchBoundary(ref_pos.y, PCType(0), range, step);
        const PCType b = _SearchBoundary(ref_pos.y, src_height - Height(), range, step);

        for (PCType j = t; j <= b; j += step)
        {
            for (PCType i = l; i <= r; i += step)
            {
                search_pos[index++] = PosType(j, i);
            }
        }
    }

    PosCode MergeSearchPos(const PosCode &src_search_pos, PosType ref_pos, PCType src_height, PCType src_width,
        PCType range, PCType step = 1) const
    {
        range = range / step * step;
        
        PosCode new_search_pos((range / step * 2 + 1) * (range / step * 2 + 1));
        size_t index = 0;
        AddSearchPos(new_search_pos, index, ref_pos, src_height, src_width, range, step);

        if (src_search_pos.size() > 0)
        {
            PosCode merge_search_pos(src_search_pos.size() + index);
            std::merge(src_search_pos.begin(), src_search_pos.end(),
                new_search_pos.begin(), new_search_pos.begin() + index, merge_search_pos.begin());

            return merge_search_pos;
        }
        else
        {
            new_search_pos.resize(index);
            return new_search_pos;
        }
    }

    PosCode GenSearchPos(const PosCode &ref_pos_code, PCType src_height, PCType src_width,
        PCType range, PCType step = 1) const
    {
        PosCode search_pos;

        for (auto ref_pos : ref_pos_code)
        {
            search_pos = MergeSearchPos(search_pos, ref_pos, src_height, src_width, range, step);
        }

        auto search_pos_nb = std::unique(search_pos.begin(), search_pos.end());
        search_pos.erase(search_pos_nb, search_pos.end());

        return search_pos;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Ty = double,
    typename _DTy = double >
class BlockGroup
{
public:
    typedef BlockGroup<_Ty, _DTy> _Myt;
    typedef _Ty value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef _Ty *pointer;
    typedef const _Ty *const_pointer;
    typedef _Ty &reference;
    typedef const _Ty &const_reference;

    typedef pointer iterator;
    typedef const_pointer const_iterator;

    typedef _DTy dist_type;

    typedef Block<value_type, dist_type> block_type;
    typedef typename block_type::KeyType KeyType;
    typedef typename block_type::PosType PosType;
    typedef typename block_type::PosPair PosPair;
    typedef typename block_type::KeyCode KeyCode;
    typedef typename block_type::PosCode PosCode;
    typedef typename block_type::PosPairCode PosPairCode;

    typedef Pos3 Pos3Type;
    typedef KeyPair<KeyType, Pos3Type> Pos3Pair;
    typedef std::vector<Pos3Type> Pos3Code;
    typedef std::vector<Pos3Pair> Pos3PairCode;

private:
    PCType GroupSize_ = 0;
    PCType Height_ = 0;
    PCType Width_ = 0;
    PCType PixelCount_ = 0;
    bool isPos3_ = false;
    PosCode posCode_;
    Pos3Code pos3Code_;
    pointer Data_ = nullptr;

public:
    template < typename _Fn1 >
    void for_each(_Fn1 _Func)
    {
        Block_For_each(*this, _Func);
    }

    template < typename _Fn1 >
    void for_each(_Fn1 _Func) const
    {
        Block_For_each(*this, _Func);
    }

    template < typename _St2, typename _Fn1 >
    void for_each(_St2 &right, _Fn1 _Func)
    {
        Block_For_each(*this, right, _Func);
    }

    template < typename _St2, typename _Fn1 >
    void for_each(_St2 &right, _Fn1 _Func) const
    {
        Block_For_each(*this, right, _Func);
    }

    template < typename _Fn1 >
    void transform(_Fn1 _Func)
    {
        Block_Transform(*this, _Func);
    }

    template < typename _St1, typename _Fn1 >
    void transform(const _St1 &src, _Fn1 _Func)
    {
        Block_Transform(*this, src, _Func);
    }

public:
    // Default constructor
    BlockGroup() {}

    BlockGroup(PCType _GroupSize, PCType _Height, PCType _Width,
        bool _isPos3 = false, bool Init = true, value_type Value = 0)
        : GroupSize_(_GroupSize), Height_(_Height), Width_(_Width),
        PixelCount_(GroupSize_ * Height_ * Width_),
        isPos3_(_isPos3)
    {
        AlignedMalloc(Data_, size());

        InitValue(Init, Value);
    }

    // Constructor from plane pointer and PosPairCode
    template < typename _St1 >
    BlockGroup(const _St1 *src, PCType src_stride, const PosPairCode &code,
        PCType _GroupSize = -1, PCType _Height = 16, PCType _Width = 16)
        : Height_(_Height), Width_(_Width)
    {
        FromCode(code, _GroupSize);

        From(src, src_stride);
    }

    // Constructor from plane pointer and Pos3PairCode
    template < typename _St1 >
    BlockGroup(const std::vector<const _St1 *> &src, PCType src_stride, const Pos3PairCode &code,
        PCType _GroupSize = -1, PCType _Height = 16, PCType _Width = 16)
        : Height_(_Height), Width_(_Width)
    {
        FromCode(code, _GroupSize);

        From(src, src_stride);
    }

    // Copy constructor
    BlockGroup(const _Myt &src)
        : GroupSize_(src.GroupSize_), Height_(src.Height_), Width_(src.Width_), PixelCount_(src.PixelCount_),
        isPos3_(src.isPos3_), posCode_(src.posCode_), pos3Code_(src.pos3Code_)
    {
        AlignedMalloc(Data_, size());

        memcpy(Data_, src.Data_, sizeof(value_type) * size());
    }

    // Move constructor
    BlockGroup(_Myt &&src)
        : GroupSize_(src.GroupSize_), Height_(src.Height_), Width_(src.Width_), PixelCount_(src.PixelCount_),
        isPos3_(src.isPos3_), posCode_(std::move(src.posCode_)), pos3Code_(std::move(src.pos3Code_))
    {
        Data_ = src.Data_;

        src.GroupSize_ = 0;
        src.Height_ = 0;
        src.Width_ = 0;
        src.PixelCount_ = 0;
        src.Data_ = nullptr;
    }

    // Destructor
    ~BlockGroup()
    {
        AlignedFree(Data_);
    }

    // Copy assignment operator
    _Myt &operator=(const _Myt &src)
    {
        if (this == &src)
        {
            return *this;
        }

        GroupSize_ = src.GroupSize_;
        Height_ = src.Height_;
        Width_ = src.Width_;
        PixelCount_ = src.PixelCount_;
        isPos3_ = src.isPos3_;
        posCode_ = src.posCode_;
        pos3Code_ = src.pos3Code_;

        memcpy(Data_, src.Data_, sizeof(value_type) * size());

        return *this;
    }

    // Move assignment operator
    _Myt &operator=(_Myt &&src)
    {
        if (this == &src)
        {
            return *this;
        }

        GroupSize_ = src.GroupSize_;
        Height_ = src.Height_;
        Width_ = src.Width_;
        PixelCount_ = src.PixelCount_;
        isPos3_ = src.isPos3_;
        posCode_ = std::move(src.posCode_);
        pos3Code_ = std::move(src.pos3Code_);

        AlignedFree(Data_);
        Data_ = src.Data_;

        src.GroupSize_ = 0;
        src.Height_ = 0;
        src.Width_ = 0;
        src.PixelCount_ = 0;
        src.Data_ = nullptr;

        return *this;
    }

    reference operator[](PCType i) { return Data_[i]; }
    const_reference operator[](PCType i) const { return Data_[i]; }
    reference operator()(PCType k, PCType j, PCType i) { return Data_[(k * Height() + j) * Stride() + i]; }
    const_reference operator()(PCType k, PCType j, PCType i) const { return Data_[(k * Height() + j) * Stride() + i]; }

    iterator begin() { return Data_; }
    const_iterator begin() const { return Data_; }
    iterator end() { return Data_ + PixelCount_; }
    const_iterator end() const { return Data_ + PixelCount_; }
    size_type size() const { return PixelCount_; }
    pointer data() { return Data_; }
    const_pointer data() const { return Data_; }
    value_type value(PCType i) { return Data_[i]; }
    const value_type value(PCType i) const { return Data_[i]; }

    PCType GroupSize() const { return GroupSize_; }
    PCType Height() const { return Height_; }
    PCType Width() const { return Width_; }
    PCType Stride() const { return Width_; }
    PCType PixelCount() const { return PixelCount_; }
    bool IsPos3() const { return isPos3_; }
    const PosCode &GetPosCode() const { return posCode_; }
    const Pos3Code &GetPos3Code() const { return pos3Code_; }
    PosType GetPos(PCType i) const { return posCode_[i]; }
    Pos3Type GetPos3(PCType i) const { return pos3Code_[i]; }

    ////////////////////////////////////////////////////////////////
    // Initialization functions

    void InitValue(bool Init = true, value_type Value = 0)
    {
        if (Init)
        {
            if (IsPos3()) posCode_.resize(GroupSize(), PosType(0, 0));
            else pos3Code_.resize(GroupSize(), Pos3Type(0, 0, 0));

            for_each([&](value_type &x)
            {
                x = Value;
            });
        }
        else
        {
            if (IsPos3()) posCode_.resize(GroupSize());
            else pos3Code_.resize(GroupSize());
        }
    }

    void FromCode(const PosPairCode &code, PCType _GroupSize = -1)
    {
        if (_GroupSize < 0)
        {
            GroupSize_ = static_cast<PCType>(code.size());
        }
        else
        {
            GroupSize_ = static_cast<PCType>(Min(code.size(), static_cast<size_type>(_GroupSize)));
        }

        PixelCount_ = GroupSize_ * Height_ * Width_;

        AlignedMalloc(Data_, size());

        posCode_.resize(GroupSize());

        for (int i = 0; i < GroupSize(); ++i)
        {
            posCode_[i] = code[i].second;
        }
    }

    void FromCode(const Pos3PairCode &code, PCType _GroupSize = -1)
    {
        if (_GroupSize < 0)
        {
            GroupSize_ = static_cast<PCType>(code.size());
        }
        else
        {
            GroupSize_ = static_cast<PCType>(Min(code.size(), static_cast<size_type>(_GroupSize)));
        }

        PixelCount_ = GroupSize_ * Height_ * Width_;

        AlignedMalloc(Data_, size());

        pos3Code_.resize(GroupSize());

        for (int i = 0; i < GroupSize(); ++i)
        {
            pos3Code_[i] = code[i].second;
        }
    }

    ////////////////////////////////////////////////////////////////
    // Read/Store functions

    template < typename _St1 >
    void From(const _St1 *src, PCType src_stride)
    {
        auto dstp = data();

        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto srcp = src + GetPos(z).y * src_stride + GetPos(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * src_stride;

                for (PCType upper = x + Width(); x < upper; ++x, ++dstp)
                {
                    *dstp = static_cast<value_type>(srcp[x]);
                }
            }
        }
    }

    template < typename _St1 >
    void From(const std::vector<const _St1 *> &src, PCType src_stride)
    {
        auto dstp = data();

        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto srcp = src[GetPos3(z).z] + GetPos3(z).y * src_stride + GetPos3(z).x;
			if(Width() % 4 == 0)
			{
				for (PCType y = 0; y < Height(); ++y)
				{
				
					register PCType x = y * src_stride;
					_mm_prefetch((const char*)&srcp[x], _MM_HINT_NTA);
					for (PCType upper = x + Width(); x < upper; x+=4, dstp+=4)
					{
						__m128 a = _mm_load_ps(&srcp[x]);
						_mm_stream_ps(dstp, a);
					
					}

				}
			}
			else
			{
				for (PCType y = 0; y < Height(); ++y)
				{

					PCType x = y * src_stride;

					for (PCType upper = x + Width()%4; x < upper; ++x, ++dstp)
					{
					 *dstp = static_cast<value_type>(srcp[x]);
					}

				}
			}
        }
    }

    template < typename _Dt1 >
    void To(_Dt1 *dst, PCType dst_stride) const
    {
        auto srcp = data();

        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto dstp = dst + GetPos(z).y * dst_stride + GetPos(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * dst_stride;

                for (PCType upper = x + Width(); x < upper; ++x, ++srcp)
                {
                    dstp[x] = static_cast<_Dt1>(*srcp);
                }
            }
        }
    }

    template < typename _Dt1 >
    void To(const std::vector<_Dt1 *> &dst, PCType dst_stride) const
    {
        auto srcp = data();

        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto dstp = dst[GetPos3(z).z] + GetPos3(z).y * dst_stride + GetPos3(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * dst_stride;

                for (PCType upper = x + Width(); x < upper; ++x, ++srcp)
                {
                    dstp[x] = static_cast<_Dt1>(*srcp);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////
    // Accumulate functions

    template < typename _Dt1 >
    void AddTo(_Dt1 *dst, PCType dst_stride) const
    {
        auto srcp = data();

        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto dstp = dst + GetPos(z).y * dst_stride + GetPos(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * dst_stride;

                for (PCType upper = x + Width(); x < upper; ++x, ++srcp)
                {
                    dstp[x] += static_cast<_Dt1>(*srcp);
                }
            }
        }
    }

    template < typename _Dt1, typename _Gt1 >
    void AddTo(_Dt1 *dst, PCType dst_stride, _Gt1 gain) const
    {
        auto srcp = data();

        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto dstp = dst + GetPos(z).y * dst_stride + GetPos(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * dst_stride;

                for (PCType upper = x + Width(); x < upper; ++x, ++srcp)
                {
                    dstp[x] += static_cast<_Dt1>(*srcp * gain);
                }
            }
        }
    }

    template < typename _Dt1 >
    void AddTo(const std::vector<_Dt1 *> &dst, PCType dst_stride) const
    {
        auto srcp = data();

        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto dstp = dst[GetPos3(z).z] + GetPos3(z).y * dst_stride + GetPos3(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * dst_stride;

                for (PCType upper = x + Width(); x < upper; ++x, ++srcp)
                {
                    dstp[x] += static_cast<_Dt1>(*srcp);
                }
            }
        }
    }

    template < typename _Dt1, typename _Gt1 >
    void AddTo(const std::vector<_Dt1 *> &dst, PCType dst_stride, _Gt1 gain) const
    {
        auto srcp = data();

        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto dstp = dst[GetPos3(z).z] + GetPos3(z).y * dst_stride + GetPos3(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * dst_stride;

                for (PCType upper = x + Width(); x < upper; ++x, ++srcp)
                {
                    dstp[x] += static_cast<_Dt1>(*srcp * gain);
                }
            }
        }
    }

    template < typename _Dt1 >
    void CountTo(_Dt1 *dst, PCType dst_stride) const
    {
        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto dstp = dst + GetPos(z).y * dst_stride + GetPos(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * dst_stride;

                for (PCType upper = x + Width(); x < upper; ++x)
                {
                    ++dstp[x];
                }
            }
        }
    }

    template < typename _Dt1 >
    void CountTo(_Dt1 *dst, PCType dst_stride, _Dt1 value) const
    {
        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto dstp = dst + GetPos(z).y * dst_stride + GetPos(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * dst_stride;

                for (PCType upper = x + Width(); x < upper; ++x)
                {
                    dstp[x] += value;
                }
            }
        }
    }

    template < typename _Dt1 >
    void CountTo(const std::vector<_Dt1 *> &dst, PCType dst_stride) const
    {
        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto dstp = dst[GetPos3(z).z] + GetPos3(z).y * dst_stride + GetPos3(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * dst_stride;

                for (PCType upper = x + Width(); x < upper; ++x)
                {
                    ++dstp[x];
                }
            }
        }
    }

    template < typename _Dt1 >
    void CountTo(const std::vector<_Dt1 *> &dst, PCType dst_stride, _Dt1 value) const
    {
        for (PCType z = 0; z < GroupSize(); ++z)
        {
            auto dstp = dst[GetPos3(z).z] + GetPos3(z).y * dst_stride + GetPos3(z).x;

            for (PCType y = 0; y < Height(); ++y)
            {
                PCType x = y * dst_stride;

                for (PCType upper = x + Width(); x < upper; ++x)
                {
                    dstp[x] += value;
                }
            }
        }
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _St1, typename _Fn1 >
void Block_For_each(_St1 &data, _Fn1 &&_Func)
{
    auto datap = data.data();

    for (auto upper = datap + data.size(); datap != upper; ++datap)
    {
        _Func(*datap);
    }
}

template < typename _St1, typename _St2, typename _Fn1 >
void Block_For_each(_St1 &data1, _St2 &data2, _Fn1 &&_Func)
{
    if (data1.size() != data2.size())
    {
        DEBUG_FAIL("Block_For_each: size() of data1 and data2 must be the same.");
    }

    auto data1p = data1.data();
    auto data2p = data2.data();

    for (auto upper = data1p + data1.size(); data1p != upper; ++data1p, ++data2p)
    {
        _Func(*data1p, *data2p);
    }
}

template < typename _St1, typename _Fn1 >
void Block_Transform(_St1 &data, _Fn1 &&_Func)
{
    auto datap = data.data();

    for (auto upper = datap + data.size(); datap != upper; ++datap)
    {
        *datap = _Func(*datap);
    }
}

template < typename _Dt1, typename _St1, typename _Fn1 >
void Block_Transform(_Dt1 &dst, const _St1 &src, _Fn1 &&_Func)
{
    if (dst.size() != src.size())
    {
        DEBUG_FAIL("Block_Transform: size() of dst and src must be the same.");
    }

    auto dstp = dst.data();
    auto srcp = src.data();

    for (auto upper = dstp + dst.size(); dstp != upper; ++dstp, ++srcp)
    {
        *dstp = _Func(*srcp);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
