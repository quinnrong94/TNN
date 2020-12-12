// Tencent is pleased to support the open source community by making TNN available.
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#ifndef TNN_EXAMPLES_UTILS_LANDMARK_H_
#define TNN_EXAMPLES_UTILS_LANDMARK_H_
#include "tnn/core/macro.h"
#include <array>
#include <iostream>
#include <vector>
#include <type_traits>
#include <string>

#define INVALID_DIM_ERROR "invalid dimmention"

namespace TNN_NS {

template <int dim, typename T>
class Landmark {
private:
    static inline T add_fun(T x, T t) {
        return x+t;
    }

    static inline T scale_fun(T x, T t) {
        return x*t;
    }

    static inline T assign_fun(T x, T t) {
        return t;
    }

public:
    using enable_if_arithmetic_T=typename std::enable_if<std::is_arithmetic<T>::value>::type ;
    template <typename=enable_if_arithmetic_T>
    Landmark():coords_({0}) {}

    template <typename... Targs, typename=enable_if_arithmetic_T>
    Landmark(Targs... args) {
        static_assert(sizeof...(Targs) == dim, INVALID_DIM_ERROR);
        Proxy(&Landmark<dim, T>::assign_fun, args...);
    }
    
    // flix the first coords_
    void FlipX(int width) {
        static_assert(dim >= 1, INVALID_DIM_ERROR);
        coords_[0] = width - coords_[0];
    }

    // add offsets
    template <typename... Targs>
    void AddOffset(Targs... args) {
        static_assert(sizeof...(Targs) == dim, INVALID_DIM_ERROR);
        Proxy(&Landmark<dim, T>::add_fun, args...);
    }

    // scale coords_
    template <typename... Targs>
    void Scale(Targs... args) {
        static_assert(sizeof...(Targs) == dim, INVALID_DIM_ERROR);
        Proxy(&Landmark<dim, T>::scale_fun, args...);
    }
    
    // get component
    T X() const {
        static_assert(dim >= 1, INVALID_DIM_ERROR);
        return coords_[0];
    }
    T Y() const {
        static_assert(dim >= 2, INVALID_DIM_ERROR);
        return coords_[1];
    }
    T Z() const {
        static_assert(dim >= 3, INVALID_DIM_ERROR);
        return coords_[2];
    }
    T Get(int idx) const {
        if (idx < dim)
            return coords_[idx];
        return 0;
    }

    // set component
    void X(T x) {
        static_assert(dim >= 1, INVALID_DIM_ERROR);
        coords_[0] = x;
    }
    void Y(T y) {
        static_assert(dim >= 2, INVALID_DIM_ERROR);
        coords_[1] = y;
    }
    void Z(T z) {
        static_assert(dim >= 3, INVALID_DIM_ERROR);
        coords_[2] = z;
    }
    void Set(int idx, T val) {
        if (idx < dim)
            coords_[idx] = val;
    }
    
    // allowing assignment throught returning lref
    T& X() {
        static_assert(dim >= 1, INVALID_DIM_ERROR);
        return coords_[0];
    }
    T& Y() {
        static_assert(dim >= 2, INVALID_DIM_ERROR);
        return coords_[1];
    }
    T& Z() {
        static_assert(dim >= 3, INVALID_DIM_ERROR);
        return coords_[2];
    }

    void Print() const {
        for(const auto&d: coords_)
            std::cout << d << '\t';
        std::cout << std::endl;
    }

    // Copy constructor, forbid different-sized copy
    Landmark(const Landmark<dim, T>& o) {
        this->coords_ = o.coords_;
    }
    Landmark& operator=(const Landmark<dim, T>& o) {
        if (this != &o) {
            this->coords_ = o.coords_;
        }
        return *this;
    }
    Landmark(Landmark<dim, T>&& o){
        this->coords_ = o.coords_;
    }

    Landmark& operator=(Landmark&& o) {
        if (this != &o) {
            this->coords_ = o.coords_;
        }
        return *this;
    }

    template <int d, typename U>
    Landmark(const Landmark<d, U>& o) = delete;
    
    template <int d, typename U>
    Landmark& operator=(const Landmark<d, U> o) = delete;
    
    template <int d, typename U>
    Landmark(Landmark<d, U>&& o) = delete;

private:
    template <typename U, typename V>
    using Fun= U (*)(U, V);

    template <typename... Targs>
    void Proxy(Fun<T, T> f, T t, Targs...args) {
        auto idx = dim-sizeof...(Targs)-1;
        coords_[idx] = f(coords_[idx], t);
        Proxy(f, args...);
    }
    
    void Proxy(Fun<T, T> f, T t) {
        coords_[dim-1] = f(coords_[dim-1], t);
    }

    std::array<T, dim> coords_;
};

typedef Landmark<2, float> Landmark2D;
typedef Landmark<3, float> Landmark3D;

}

#endif // TNN_EXAMPLES_UTILS_LANDMARK_H_
