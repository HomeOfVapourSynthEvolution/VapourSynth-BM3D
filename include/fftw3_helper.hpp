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


#ifndef FFTW3_HELPER_HPP_
#define FFTW3_HELPER_HPP_


#include <fftw3.h>


template < typename R = double >
struct fftwh
{
    typedef ::fftw_complex complex;
    typedef ::fftw_plan_s plan_s;
    typedef ::fftw_iodim64 iodim;
    typedef ::fftw_r2r_kind r2r_kind;
    typedef ::fftw_write_char_func write_char_func;
    typedef ::fftw_read_char_func read_char_func;


    typedef R RType;
    typedef complex CType;


    struct plan
    {
        plan_s *p = nullptr;


        plan()
        {}

        plan(const plan &right) = delete;

        plan(plan &&right)
            : p(right.p)
        {
            right.p = nullptr;
        }

        ~plan()
        {
            destroy_plan();
        }

        plan &operator=(const plan &right) = delete;

        plan &operator=(plan &&right)
        {
            destroy_plan();
            p = right.p;
            right.p = nullptr;
            return *this;
        }


        void execute() const
        {
            ::fftw_execute(p);
        }


        template < typename C >
        void dft(int rank, const int *n, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft(rank, n, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }


        template < typename C >
        void dft_1d(int n, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_1d(n, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }

        template < typename C >
        void dft_2d(int n0, int n1, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_2d(n0, n1, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }

        template < typename C >
        void dft_3d(int n0, int n1, int n2, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_3d(n0, n1, n2, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }


        template < typename C >
        void many_dft(int rank, const int *n, int howmany, C *in, const int *inembed, int istride, int idist,
            C *out, const int *onembed, int ostride, int odist,
            int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_many_dft(rank, n, howmany, reinterpret_cast<CType *>(in), inembed, istride, idist,
                reinterpret_cast<CType *>(out), onembed, ostride, odist, sign, flags);
        }


        template < typename C >
        void guru_dft(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_guru_dft(rank, dims, howmany_rank, howmany_dims,
                reinterpret_cast<CType *>(in), reinterpret_cast<CType *>(out), sign, flags);
        }

        void guru_split_dft(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            R *ri, R *ii, R *ro, R *io, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftw_plan_guru_split_dft(rank, dims, howmany_rank, howmany_dims, ri, ii, ro, io, flags);
        }


        template < typename C >
        void execute_dft(C *in, C *out) const
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            ::fftw_execute_dft(p, reinterpret_cast<CType *>(in), reinterpret_cast<CType *>(out));
        }

        void execute_split_dft(R *ri, R *ii, R *ro, R *io) const
        {
            ::fftw_execute_split_dft(p, ri, ii, ro, io);
        }


        template < typename C >
        void many_dft_r2c(int rank, const int *n, int howmany,
            R *in, const int *inembed, int istride, int idist,
            C *out, const int *onembed, int ostride, int odist,
            unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_many_dft_r2c(rank, n, howmany, in, inembed, istride, idist,
                reinterpret_cast<CType *>(out), onembed, ostride, odist, flags);
        }


        template < typename C >
        void dft_r2c(int rank, const int *n, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_r2c(rank, n, in, reinterpret_cast<CType *>(out), flags);
        }


        template < typename C >
        void dft_r2c_1d(int n, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_r2c_1d(n, in, reinterpret_cast<CType *>(out), flags);
        }

        template < typename C >
        void dft_r2c_2d(int n0, int n1, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_r2c_2d(n0, n1, in, reinterpret_cast<CType *>(out), flags);
        }

        template < typename C >
        void dft_r2c_3d(int n0, int n1, int n2, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_r2c_3d(n0, n1, n2, in, reinterpret_cast<CType *>(out), flags);
        }


        template < typename C >
        void many_dft_c2r(int rank, const int *n, int howmany,
            C *in, const int *inembed, int istride, int idist,
            R *out, const int *onembed, int ostride, int odist,
            unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_many_dft_c2r(rank, n, howmany,
                reinterpret_cast<CType *>(in), inembed, istride, idist,
                out, onembed, ostride, odist, flags);
        }


        template < typename C >
        void dft_c2r(int rank, const int *n, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_c2r(rank, n, reinterpret_cast<CType *>(in), out, flags);
        }


        template < typename C >
        void dft_c2r_1d(int n, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_c2r_1d(n, reinterpret_cast<CType *>(in), out, flags);
        }

        template < typename C >
        void dft_c2r_2d(int n0, int n1, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_c2r_2d(n0, n1, reinterpret_cast<CType *>(in), out, flags);
        }

        template < typename C >
        void dft_c2r_3d(int n0, int n1, int n2, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_dft_c2r_3d(n0, n1, n2, reinterpret_cast<CType *>(in), out, flags);
        }


        template < typename C >
        void guru_dft_r2c(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_guru_dft_r2c(rank, dims, howmany_rank, howmany_dims,
                in, reinterpret_cast<CType *>(out), flags);
        }

        template < typename C >
        void guru_dft_c2r(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftw_plan_guru_dft_c2r(rank, dims, howmany_rank, howmany_dims,
                reinterpret_cast<CType *>(in), out, flags);
        }


        void guru_split_dft_r2c(int rank, const iodim *dims,
            int howmany_rank, const iodim *howmany_dims,
            R *in, R *ro, R *io, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftw_plan_guru_split_dft_r2c(rank, dims, howmany_rank, howmany_dims, in, ro, io, flags);
        }

        void guru_split_dft_c2r(int rank, const iodim *dims,
            int howmany_rank, const iodim *howmany_dims,
            R *ri, R *ii, R *out, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftw_plan_guru_split_dft_c2r(rank, dims, howmany_rank, howmany_dims, ri, ii, out, flags);
        }


        template < typename C >
        void execute_dft_r2c(R *in, C *out) const
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            ::fftw_execute_dft_r2c(p, in, reinterpret_cast<CType *>(out));
        }

        template < typename C >
        void execute_dft_c2r(C *in, R *out) const
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            ::fftw_execute_dft_c2r(p, reinterpret_cast<CType *>(in), out);
        }


        void execute_split_dft_r2c(R *in, R *ro, R *io) const
        {
            ::fftw_execute_split_dft_r2c(p, in, ro, io);
        }

        void execute_split_dft_c2r(R *ri, R *ii, R *out) const
        {
            ::fftw_execute_split_dft_c2r(p, ri, ii, out);
        }


        void many_r2r(int rank, const int *n, int howmany,
            R *in, const int *inembed, int istride, int idist,
            R *out, const int *onembed, int ostride, int odist,
            const r2r_kind *kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftw_plan_many_r2r(rank, n, howmany, in, inembed, istride, idist,
                out, onembed, ostride, odist, kind, flags);
        }


        void r2r(int rank, const int *n, R *in, R *out, const r2r_kind *kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftw_plan_r2r(rank, n, in, out, kind, flags);
        }


        void r2r_1d(int n, R *in, R *out, r2r_kind kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftw_plan_r2r_1d(n, in, out, kind, flags);
        }

        void r2r_2d(int n0, int n1, R *in, R *out,
            r2r_kind kind0, r2r_kind kind1, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftw_plan_r2r_2d(n0, n1, in, out, kind0, kind1, flags);
        }

        void r2r_3d(int n0, int n1, int n2, R *in, R *out,
            r2r_kind kind0, r2r_kind kind1, r2r_kind kind2, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftw_plan_r2r_3d(n0, n1, n2, in, out, kind0, kind1, kind2, flags);
        }


        void guru_r2r(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            R *in, R *out, const r2r_kind *kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftw_plan_guru_r2r(rank, dims, howmany_rank, howmany_dims, in, out, kind, flags);
        }


        void execute_r2r(R *in, R *out) const
        {
            ::fftw_execute_r2r(p, in, out);
        }


        void destroy_plan()
        {
            if (p != nullptr)
            {
                ::fftw_destroy_plan(p);
                p = nullptr;
            }
        }


        void fprint_plan(FILE *output_file) const
        {
            ::fftw_fprint_plan(p, output_file);
        }

        void print_plan() const
        {
            ::fftw_print_plan(p);
        }

        char *sprint_plan() const
        {
            return ::fftw_sprint_plan(p);
        }


        void flops(double *add, double *mul, double *fmas) const
        {
            ::fftw_flops(p, add, mul, fmas);
        }

        double estimate_cost() const
        {
            return ::fftw_estimate_cost(p);
        }

        double cost() const
        {
            return ::fftw_cost(p);
        }
    };


    static void forget_wisdom()
    {
        ::fftw_forget_wisdom();
    }

    static void cleanup()
    {
        ::fftw_cleanup();
    }


    static void set_timelimit(double t)
    {
        ::fftw_set_timelimit(t);
    }


    static void plan_with_nthreads(int nthreads)
    {
        ::fftw_plan_with_nthreads(nthreads);
    }

    static void init_threads()
    {
        ::fftw_init_threads();
    }

    static void cleanup_threads()
    {
        ::fftw_cleanup_threads();
    }


    static void export_wisdom_to_filename(const char *filename)
    {
        ::fftw_export_wisdom_to_filename(filename);
    }

    static void export_wisdom_to_file(FILE *output_file)
    {
        ::fftw_export_wisdom_to_file(output_file);
    }

    static char *export_wisdom_to_string()
    {
        return ::fftw_export_wisdom_to_string();
    }

    template < typename _Ty >
    static void export_wisdom(write_char_func write_char, _Ty *data)
    {
        ::fftw_export_wisdom(write_char, reinterpret_cast<void *>(data));
    }

    static int import_system_wisdom()
    {
        return ::fftw_import_system_wisdom();
    }

    static int import_wisdom_from_filename(const char *filename)
    {
        return ::fftw_import_wisdom_from_filename(filename);
    }

    static int import_wisdom_from_file(FILE *input_file)
    {
        return ::fftw_import_wisdom_from_file(input_file);
    }

    static int import_wisdom_from_string(const char *input_string)
    {
        return ::fftw_import_wisdom_from_string(input_string);
    }

    template < typename _Ty >
    static int import_wisdom(read_char_func read_char, _Ty *data)
    {
        return ::fftw_import_wisdom(read_char, reinterpret_cast<void *>(data));
    }


    template < typename _Ty >
    static void malloc(_Ty *&p, size_t count)
    {
        p = reinterpret_cast<_Ty *>(::fftw_malloc(sizeof(_Ty) * count));
    }

    static RType *alloc_real(size_t count)
    {
        return ::fftw_alloc_real(sizeof(RType) * count);
    }

    static CType *alloc_complex(size_t count)
    {
        return ::fftw_alloc_complex(sizeof(CType) * count);
    }

    template < typename _Ty >
    static void free(_Ty *&p)
    {
        ::fftw_free(p);
        p = nullptr;
    }


    template < typename _Ty >
    static int alignment_of(_Ty *p)
    {
        return ::fftw_alignment_of(reinterpret_cast<R *>(p));
    }
};


template <>
struct fftwh<float>
{
    typedef float R;


    typedef ::fftwf_complex complex;
    typedef ::fftwf_plan_s plan_s;
    typedef ::fftwf_iodim64 iodim;
    typedef ::fftwf_r2r_kind r2r_kind;
    typedef ::fftwf_write_char_func write_char_func;
    typedef ::fftwf_read_char_func read_char_func;


    typedef R RType;
    typedef complex CType;


    struct plan
    {
        plan_s *p = nullptr;


        plan()
        {}

        plan(const plan &right) = delete;

        plan(plan &&right)
            : p(right.p)
        {
            right.p = nullptr;
        }

        ~plan()
        {
            destroy_plan();
        }

        plan &operator=(const plan &right) = delete;

        plan &operator=(plan &&right)
        {
            destroy_plan();
            p = right.p;
            right.p = nullptr;
            return *this;
        }


        void execute() const
        {
            ::fftwf_execute(p);
        }


        template < typename C >
        void dft(int rank, const int *n, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft(rank, n, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }


        template < typename C >
        void dft_1d(int n, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_1d(n, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }

        template < typename C >
        void dft_2d(int n0, int n1, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_2d(n0, n1, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }

        template < typename C >
        void dft_3d(int n0, int n1, int n2, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_3d(n0, n1, n2, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }


        template < typename C >
        void many_dft(int rank, const int *n, int howmany, C *in, const int *inembed, int istride, int idist,
            C *out, const int *onembed, int ostride, int odist,
            int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_many_dft(rank, n, howmany, reinterpret_cast<CType *>(in), inembed, istride, idist,
                reinterpret_cast<CType *>(out), onembed, ostride, odist, sign, flags);
        }


        template < typename C >
        void guru_dft(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_guru64_dft(rank, dims, howmany_rank, howmany_dims,
                reinterpret_cast<CType *>(in), reinterpret_cast<CType *>(out), sign, flags);
        }

        void guru_split_dft(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            R *ri, R *ii, R *ro, R *io, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwf_plan_guru64_split_dft(rank, dims, howmany_rank, howmany_dims, ri, ii, ro, io, flags);
        }


        template < typename C >
        void execute_dft(C *in, C *out) const
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            ::fftwf_execute_dft(p, reinterpret_cast<CType *>(in), reinterpret_cast<CType *>(out));
        }

        void execute_split_dft(R *ri, R *ii, R *ro, R *io) const
        {
            ::fftwf_execute_split_dft(p, ri, ii, ro, io);
        }


        template < typename C >
        void many_dft_r2c(int rank, const int *n, int howmany,
            R *in, const int *inembed, int istride, int idist,
            C *out, const int *onembed, int ostride, int odist,
            unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_many_dft_r2c(rank, n, howmany, in, inembed, istride, idist,
                reinterpret_cast<CType *>(out), onembed, ostride, odist, flags);
        }


        template < typename C >
        void dft_r2c(int rank, const int *n, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_r2c(rank, n, in, reinterpret_cast<CType *>(out), flags);
        }


        template < typename C >
        void dft_r2c_1d(int n, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_r2c_1d(n, in, reinterpret_cast<CType *>(out), flags);
        }

        template < typename C >
        void dft_r2c_2d(int n0, int n1, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_r2c_2d(n0, n1, in, reinterpret_cast<CType *>(out), flags);
        }

        template < typename C >
        void dft_r2c_3d(int n0, int n1, int n2, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_r2c_3d(n0, n1, n2, in, reinterpret_cast<CType *>(out), flags);
        }


        template < typename C >
        void many_dft_c2r(int rank, const int *n, int howmany,
            C *in, const int *inembed, int istride, int idist,
            R *out, const int *onembed, int ostride, int odist,
            unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_many_dft_c2r(rank, n, howmany,
                reinterpret_cast<CType *>(in), inembed, istride, idist,
                out, onembed, ostride, odist, flags);
        }


        template < typename C >
        void dft_c2r(int rank, const int *n, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_c2r(rank, n, reinterpret_cast<CType *>(in), out, flags);
        }


        template < typename C >
        void dft_c2r_1d(int n, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_c2r_1d(n, reinterpret_cast<CType *>(in), out, flags);
        }

        template < typename C >
        void dft_c2r_2d(int n0, int n1, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_c2r_2d(n0, n1, reinterpret_cast<CType *>(in), out, flags);
        }

        template < typename C >
        void dft_c2r_3d(int n0, int n1, int n2, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_dft_c2r_3d(n0, n1, n2, reinterpret_cast<CType *>(in), out, flags);
        }


        template < typename C >
        void guru_dft_r2c(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_guru64_dft_r2c(rank, dims, howmany_rank, howmany_dims,
                in, reinterpret_cast<CType *>(out), flags);
        }

        template < typename C >
        void guru_dft_c2r(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwf_plan_guru64_dft_c2r(rank, dims, howmany_rank, howmany_dims,
                reinterpret_cast<CType *>(in), out, flags);
        }


        void guru_split_dft_r2c(int rank, const iodim *dims,
            int howmany_rank, const iodim *howmany_dims,
            R *in, R *ro, R *io, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwf_plan_guru64_split_dft_r2c(rank, dims, howmany_rank, howmany_dims, in, ro, io, flags);
        }

        void guru_split_dft_c2r(int rank, const iodim *dims,
            int howmany_rank, const iodim *howmany_dims,
            R *ri, R *ii, R *out, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwf_plan_guru64_split_dft_c2r(rank, dims, howmany_rank, howmany_dims, ri, ii, out, flags);
        }


        template < typename C >
        void execute_dft_r2c(R *in, C *out) const
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            ::fftwf_execute_dft_r2c(p, in, reinterpret_cast<CType *>(out));
        }

        template < typename C >
        void execute_dft_c2r(C *in, R *out) const
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            ::fftwf_execute_dft_c2r(p, reinterpret_cast<CType *>(in), out);
        }


        void execute_split_dft_r2c(R *in, R *ro, R *io) const
        {
            ::fftwf_execute_split_dft_r2c(p, in, ro, io);
        }

        void execute_split_dft_c2r(R *ri, R *ii, R *out) const
        {
            ::fftwf_execute_split_dft_c2r(p, ri, ii, out);
        }


        void many_r2r(int rank, const int *n, int howmany,
            R *in, const int *inembed, int istride, int idist,
            R *out, const int *onembed, int ostride, int odist,
            const r2r_kind *kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwf_plan_many_r2r(rank, n, howmany, in, inembed, istride, idist,
                out, onembed, ostride, odist, kind, flags);
        }


        void r2r(int rank, const int *n, R *in, R *out, const r2r_kind *kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwf_plan_r2r(rank, n, in, out, kind, flags);
        }


        void r2r_1d(int n, R *in, R *out, r2r_kind kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwf_plan_r2r_1d(n, in, out, kind, flags);
        }

        void r2r_2d(int n0, int n1, R *in, R *out,
            r2r_kind kind0, r2r_kind kind1, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwf_plan_r2r_2d(n0, n1, in, out, kind0, kind1, flags);
        }

        void r2r_3d(int n0, int n1, int n2, R *in, R *out,
            r2r_kind kind0, r2r_kind kind1, r2r_kind kind2, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwf_plan_r2r_3d(n0, n1, n2, in, out, kind0, kind1, kind2, flags);
        }


        void guru_r2r(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            R *in, R *out, const r2r_kind *kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwf_plan_guru64_r2r(rank, dims, howmany_rank, howmany_dims, in, out, kind, flags);
        }


        void execute_r2r(R *in, R *out) const
        {
            ::fftwf_execute_r2r(p, in, out);
        }


        void destroy_plan()
        {
            if (p != nullptr)
            {
                ::fftwf_destroy_plan(p);
                p = nullptr;
            }
        }


        void fprint_plan(FILE *output_file) const
        {
            ::fftwf_fprint_plan(p, output_file);
        }

        void print_plan() const
        {
            ::fftwf_print_plan(p);
        }

        char *sprint_plan() const
        {
            return ::fftwf_sprint_plan(p);
        }


        void flops(double *add, double *mul, double *fmas) const
        {
            ::fftwf_flops(p, add, mul, fmas);
        }

        double estimate_cost() const
        {
            return ::fftwf_estimate_cost(p);
        }

        double cost() const
        {
            return ::fftwf_cost(p);
        }
    };


    static void forget_wisdom()
    {
        ::fftwf_forget_wisdom();
    }

    static void cleanup()
    {
        ::fftwf_cleanup();
    }


    static void set_timelimit(double t)
    {
        ::fftwf_set_timelimit(t);
    }


    static void plan_with_nthreads(int nthreads)
    {
        ::fftwf_plan_with_nthreads(nthreads);
    }

    static void init_threads()
    {
        ::fftwf_init_threads();
    }

    static void cleanup_threads()
    {
        ::fftwf_cleanup_threads();
    }


    static void export_wisdom_to_filename(const char *filename)
    {
        ::fftwf_export_wisdom_to_filename(filename);
    }

    static void export_wisdom_to_file(FILE *output_file)
    {
        ::fftwf_export_wisdom_to_file(output_file);
    }

    static char *export_wisdom_to_string()
    {
        return ::fftwf_export_wisdom_to_string();
    }

    template < typename _Ty >
    static void export_wisdom(write_char_func write_char, _Ty *data)
    {
        ::fftwf_export_wisdom(write_char, reinterpret_cast<void *>(data));
    }

    static int import_system_wisdom()
    {
        return ::fftwf_import_system_wisdom();
    }

    static int import_wisdom_from_filename(const char *filename)
    {
        return ::fftwf_import_wisdom_from_filename(filename);
    }

    static int import_wisdom_from_file(FILE *input_file)
    {
        return ::fftwf_import_wisdom_from_file(input_file);
    }

    static int import_wisdom_from_string(const char *input_string)
    {
        return ::fftwf_import_wisdom_from_string(input_string);
    }

    template < typename _Ty >
    static int import_wisdom(read_char_func read_char, _Ty *data)
    {
        return ::fftwf_import_wisdom(read_char, reinterpret_cast<void *>(data));
    }


    template < typename _Ty >
    static void malloc(_Ty *&p, size_t count)
    {
        p = reinterpret_cast<_Ty *>(::fftwf_malloc(sizeof(_Ty) * count));
    }

    static RType *alloc_real(size_t count)
    {
        return ::fftwf_alloc_real(sizeof(RType) * count);
    }

    static CType *alloc_complex(size_t count)
    {
        return ::fftwf_alloc_complex(sizeof(CType) * count);
    }

    template < typename _Ty >
    static void free(_Ty *&p)
    {
        ::fftwf_free(p);
        p = nullptr;
    }


    template < typename _Ty >
    static int alignment_of(_Ty *p)
    {
        return ::fftwf_alignment_of(reinterpret_cast<R *>(p));
    }
};


template <>
struct fftwh<long double>
{
    typedef long double R;


    typedef ::fftwl_complex complex;
    typedef ::fftwl_plan_s plan_s;
    typedef ::fftwl_iodim64 iodim;
    typedef ::fftwl_r2r_kind r2r_kind;
    typedef ::fftwl_write_char_func write_char_func;
    typedef ::fftwl_read_char_func read_char_func;


    typedef R RType;
    typedef complex CType;


    struct plan
    {
        plan_s *p = nullptr;


        plan()
        {}

        plan(const plan &right) = delete;

        plan(plan &&right)
            : p(right.p)
        {
            right.p = nullptr;
        }

        ~plan()
        {
            destroy_plan();
        }

        plan &operator=(const plan &right) = delete;

        plan &operator=(plan &&right)
        {
            destroy_plan();
            p = right.p;
            right.p = nullptr;
            return *this;
        }


        void execute() const
        {
            ::fftwl_execute(p);
        }


        template < typename C >
        void dft(int rank, const int *n, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft(rank, n, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }


        template < typename C >
        void dft_1d(int n, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_1d(n, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }

        template < typename C >
        void dft_2d(int n0, int n1, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_2d(n0, n1, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }

        template < typename C >
        void dft_3d(int n0, int n1, int n2, C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_3d(n0, n1, n2, reinterpret_cast<CType *>(in),
                reinterpret_cast<CType *>(out), sign, flags);
        }


        template < typename C >
        void many_dft(int rank, const int *n, int howmany, C *in, const int *inembed, int istride, int idist,
            C *out, const int *onembed, int ostride, int odist,
            int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_many_dft(rank, n, howmany, reinterpret_cast<CType *>(in), inembed, istride, idist,
                reinterpret_cast<CType *>(out), onembed, ostride, odist, sign, flags);
        }


        template < typename C >
        void guru_dft(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            C *in, C *out, int sign = FFTW_FORWARD, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_guru64_dft(rank, dims, howmany_rank, howmany_dims,
                reinterpret_cast<CType *>(in), reinterpret_cast<CType *>(out), sign, flags);
        }

        void guru_split_dft(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            R *ri, R *ii, R *ro, R *io, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwl_plan_guru64_split_dft(rank, dims, howmany_rank, howmany_dims, ri, ii, ro, io, flags);
        }


        template < typename C >
        void execute_dft(C *in, C *out) const
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            ::fftwl_execute_dft(p, reinterpret_cast<CType *>(in), reinterpret_cast<CType *>(out));
        }

        void execute_split_dft(R *ri, R *ii, R *ro, R *io) const
        {
            ::fftwl_execute_split_dft(p, ri, ii, ro, io);
        }


        template < typename C >
        void many_dft_r2c(int rank, const int *n, int howmany,
            R *in, const int *inembed, int istride, int idist,
            C *out, const int *onembed, int ostride, int odist,
            unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_many_dft_r2c(rank, n, howmany, in, inembed, istride, idist,
                reinterpret_cast<CType *>(out), onembed, ostride, odist, flags);
        }


        template < typename C >
        void dft_r2c(int rank, const int *n, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_r2c(rank, n, in, reinterpret_cast<CType *>(out), flags);
        }


        template < typename C >
        void dft_r2c_1d(int n, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_r2c_1d(n, in, reinterpret_cast<CType *>(out), flags);
        }

        template < typename C >
        void dft_r2c_2d(int n0, int n1, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_r2c_2d(n0, n1, in, reinterpret_cast<CType *>(out), flags);
        }

        template < typename C >
        void dft_r2c_3d(int n0, int n1, int n2, R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_r2c_3d(n0, n1, n2, in, reinterpret_cast<CType *>(out), flags);
        }


        template < typename C >
        void many_dft_c2r(int rank, const int *n, int howmany,
            C *in, const int *inembed, int istride, int idist,
            R *out, const int *onembed, int ostride, int odist,
            unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_many_dft_c2r(rank, n, howmany,
                reinterpret_cast<CType *>(in), inembed, istride, idist,
                out, onembed, ostride, odist, flags);
        }


        template < typename C >
        void dft_c2r(int rank, const int *n, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_c2r(rank, n, reinterpret_cast<CType *>(in), out, flags);
        }


        template < typename C >
        void dft_c2r_1d(int n, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_c2r_1d(n, reinterpret_cast<CType *>(in), out, flags);
        }

        template < typename C >
        void dft_c2r_2d(int n0, int n1, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_c2r_2d(n0, n1, reinterpret_cast<CType *>(in), out, flags);
        }

        template < typename C >
        void dft_c2r_3d(int n0, int n1, int n2, C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_dft_c2r_3d(n0, n1, n2, reinterpret_cast<CType *>(in), out, flags);
        }


        template < typename C >
        void guru_dft_r2c(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            R *in, C *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_guru64_dft_r2c(rank, dims, howmany_rank, howmany_dims,
                in, reinterpret_cast<CType *>(out), flags);
        }

        template < typename C >
        void guru_dft_c2r(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            C *in, R *out, unsigned flags = FFTW_MEASURE)
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            destroy_plan();
            p = ::fftwl_plan_guru64_dft_c2r(rank, dims, howmany_rank, howmany_dims,
                reinterpret_cast<CType *>(in), out, flags);
        }


        void guru_split_dft_r2c(int rank, const iodim *dims,
            int howmany_rank, const iodim *howmany_dims,
            R *in, R *ro, R *io, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwl_plan_guru64_split_dft_r2c(rank, dims, howmany_rank, howmany_dims, in, ro, io, flags);
        }

        void guru_split_dft_c2r(int rank, const iodim *dims,
            int howmany_rank, const iodim *howmany_dims,
            R *ri, R *ii, R *out, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwl_plan_guru64_split_dft_c2r(rank, dims, howmany_rank, howmany_dims, ri, ii, out, flags);
        }


        template < typename C >
        void execute_dft_r2c(R *in, C *out) const
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            ::fftwl_execute_dft_r2c(p, in, reinterpret_cast<CType *>(out));
        }

        template < typename C >
        void execute_dft_c2r(C *in, R *out) const
        {
            static_assert(sizeof(C) == sizeof(CType), "Invalid arguments for template instantiation!");
            ::fftwl_execute_dft_c2r(p, reinterpret_cast<CType *>(in), out);
        }


        void execute_split_dft_r2c(R *in, R *ro, R *io) const
        {
            ::fftwl_execute_split_dft_r2c(p, in, ro, io);
        }

        void execute_split_dft_c2r(R *ri, R *ii, R *out) const
        {
            ::fftwl_execute_split_dft_c2r(p, ri, ii, out);
        }


        void many_r2r(int rank, const int *n, int howmany,
            R *in, const int *inembed, int istride, int idist,
            R *out, const int *onembed, int ostride, int odist,
            const r2r_kind *kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwl_plan_many_r2r(rank, n, howmany, in, inembed, istride, idist,
                out, onembed, ostride, odist, kind, flags);
        }


        void r2r(int rank, const int *n, R *in, R *out, const r2r_kind *kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwl_plan_r2r(rank, n, in, out, kind, flags);
        }


        void r2r_1d(int n, R *in, R *out, r2r_kind kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwl_plan_r2r_1d(n, in, out, kind, flags);
        }

        void r2r_2d(int n0, int n1, R *in, R *out,
            r2r_kind kind0, r2r_kind kind1, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwl_plan_r2r_2d(n0, n1, in, out, kind0, kind1, flags);
        }

        void r2r_3d(int n0, int n1, int n2, R *in, R *out,
            r2r_kind kind0, r2r_kind kind1, r2r_kind kind2, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwl_plan_r2r_3d(n0, n1, n2, in, out, kind0, kind1, kind2, flags);
        }


        void guru_r2r(int rank, const iodim *dims, int howmany_rank, const iodim *howmany_dims,
            R *in, R *out, const r2r_kind *kind, unsigned flags = FFTW_MEASURE)
        {
            destroy_plan();
            p = ::fftwl_plan_guru64_r2r(rank, dims, howmany_rank, howmany_dims, in, out, kind, flags);
        }


        void execute_r2r(R *in, R *out) const
        {
            ::fftwl_execute_r2r(p, in, out);
        }


        void destroy_plan()
        {
            if (p != nullptr)
            {
                ::fftwl_destroy_plan(p);
                p = nullptr;
            }
        }


        void fprint_plan(FILE *output_file) const
        {
            ::fftwl_fprint_plan(p, output_file);
        }

        void print_plan() const
        {
            ::fftwl_print_plan(p);
        }

        char *sprint_plan() const
        {
            return ::fftwl_sprint_plan(p);
        }


        void flops(double *add, double *mul, double *fmas) const
        {
            ::fftwl_flops(p, add, mul, fmas);
        }

        double estimate_cost() const
        {
            return ::fftwl_estimate_cost(p);
        }

        double cost() const
        {
            return ::fftwl_cost(p);
        }
    };


    static void forget_wisdom()
    {
        ::fftwl_forget_wisdom();
    }

    static void cleanup()
    {
        ::fftwl_cleanup();
    }


    static void set_timelimit(double t)
    {
        ::fftwl_set_timelimit(t);
    }


    static void plan_with_nthreads(int nthreads)
    {
        ::fftwl_plan_with_nthreads(nthreads);
    }

    static void init_threads()
    {
        ::fftwl_init_threads();
    }

    static void cleanup_threads()
    {
        ::fftwl_cleanup_threads();
    }


    static void export_wisdom_to_filename(const char *filename)
    {
        ::fftwl_export_wisdom_to_filename(filename);
    }

    static void export_wisdom_to_file(FILE *output_file)
    {
        ::fftwl_export_wisdom_to_file(output_file);
    }

    static char *export_wisdom_to_string()
    {
        return ::fftwl_export_wisdom_to_string();
    }

    template < typename _Ty >
    static void export_wisdom(write_char_func write_char, _Ty *data)
    {
        ::fftwl_export_wisdom(write_char, reinterpret_cast<void *>(data));
    }

    static int import_system_wisdom()
    {
        return ::fftwl_import_system_wisdom();
    }

    static int import_wisdom_from_filename(const char *filename)
    {
        return ::fftwl_import_wisdom_from_filename(filename);
    }

    static int import_wisdom_from_file(FILE *input_file)
    {
        return ::fftwl_import_wisdom_from_file(input_file);
    }

    static int import_wisdom_from_string(const char *input_string)
    {
        return ::fftwl_import_wisdom_from_string(input_string);
    }

    template < typename _Ty >
    static int import_wisdom(read_char_func read_char, _Ty *data)
    {
        return ::fftwl_import_wisdom(read_char, reinterpret_cast<void *>(data));
    }


    template < typename _Ty >
    static void malloc(_Ty *&p, size_t count)
    {
        p = reinterpret_cast<_Ty *>(::fftwl_malloc(sizeof(_Ty) * count));
    }

    static RType *alloc_real(size_t count)
    {
        return ::fftwl_alloc_real(sizeof(RType) * count);
    }

    static CType *alloc_complex(size_t count)
    {
        return ::fftwl_alloc_complex(sizeof(CType) * count);
    }

    template < typename _Ty >
    static void free(_Ty *&p)
    {
        ::fftwl_free(p);
        p = nullptr;
    }


    template < typename _Ty >
    static int alignment_of(_Ty *p)
    {
        return ::fftwl_alignment_of(reinterpret_cast<R *>(p));
    }
};


typedef fftwh<double> fftw;
typedef fftwh<float> fftwf;
typedef fftwh<long double> fftwl;


#endif
