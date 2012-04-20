// UTCommon.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Runtime/Runtime.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <string.h>
#include <fstream>
#include <iomanip>

namespace ut
{
    class spaceindent;
    class tabindent;
    class pattern;
    class filllevel;

    template <class T, class TraitsT>
    std::basic_ostream<T, TraitsT>& operator<<(std::basic_ostream<T, TraitsT> &os, const spaceindent &indent);

    template <class T, class TraitsT>
    std::basic_ostream<T, TraitsT>& operator<<(std::basic_ostream<T, TraitsT> &os, const tabindent &indent);

    template <class T, class TraitsT>
    std::basic_ostream<T, TraitsT>& operator<<(std::basic_ostream<T, TraitsT> &os, const pattern &p);

    class spaceindent
    {

    public:

        spaceindent(int spaces);

        template <class T, class TraitsT>
        friend std::basic_ostream<T, TraitsT>& operator<<(std::basic_ostream<T, TraitsT>& os, const spaceindent &indent);

    private:

        int m_spaces;

    };

    class tabindent
    {

    public:

        tabindent(int tabs);

        template <class T, class TraitsT>
        friend std::basic_ostream<T, TraitsT>& operator<<(std::basic_ostream<T, TraitsT>& os, const tabindent &indent);

    private:

        int m_tabs;

    };

    class pattern
    {

    public:

        pattern(const char *sequence, int repeat);

        template <class T, class TraitsT>
        friend std::basic_ostream<T, TraitsT>& operator<<(std::basic_ostream<T, TraitsT>& os, const pattern &p);

    private:

        const char *m_sequence;
        int         m_repeat;

    };

    class filllevel :
        public spaceindent
    {

    public:

        filllevel(int l) : spaceindent(l * 2) {}

    };

    template <class T, class TraitsT>
    inline std::basic_ostream<T, TraitsT>& operator<<(std::basic_ostream<T, TraitsT> &os, const spaceindent &indent)
    {
        for (int i = 0; i < indent.m_spaces; i++) { os << ' '; };
        return os;
    }

    template <class T, class TraitsT>
    inline std::basic_ostream<T, TraitsT>& operator<<(std::basic_ostream<T, TraitsT> &os, const tabindent &indent)
    {
        for (int i = 0; i < indent.m_tabs; i++) { os << '\t'; };
        return os;
    }

    template <class T, class TraitsT>
    inline std::basic_ostream<T, TraitsT>& operator<<(std::basic_ostream<T, TraitsT> &os, const pattern &p)
    {
        for (int i = 0; i < p.m_repeat; i++) { os << p.m_sequence; };
        return os;
    }

    inline spaceindent::spaceindent(int spaces) :
    m_spaces(spaces)
    {
    }

    inline tabindent::tabindent(int tabs) :
    m_tabs(tabs)
    {
    }

    inline pattern::pattern(const char *sequence, int repeat) :
    m_sequence(sequence),
        m_repeat(repeat)
    {
    }

    namespace
    {
        std::string s_name;
        int         s_code;
    }

    inline void Begin(const char *name)
    {
        std::cout << "******* " << name << " *******" << std::endl;
        s_name = name;
        s_code  = 0;
    }

    inline int Fail(int code)
    {
        s_code = code;
        return code;
    }

    inline int Fail(int code, const char *fmt, ...)
    {
        s_code = code;
        char buff[1024];
        va_list l;
        va_start(l, fmt);
        vsprintf(buff, fmt, l);
        va_end(l);
        std::cout << s_name.c_str() << " failed." << std::endl;
        std::cout << "\t code: " << code << std::endl;
        if (buff[0]) { std::cout << "\t  msg: " << buff << std::endl; }
        return code;
    }

    inline int Code() { return s_code; }

#define FAIL(...) ut::Fail(__VA_ARGS__); return
#define INIT() int f = 0; int s = 0
#define RUN(_n, _x) if (argc<2 || testToRun == _n) { _x; if (ut::Code()<0) { ++f; } else { ++s; } }
#define DO(_x) if (ut::Code()>=0) { _x; }
#define END() std::wcout << "------" << std::endl << s << " succeeded, " << f << " failed." << std::endl; return (f>0) ? -1 : 0
} // ut
