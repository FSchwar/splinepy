#pragma once

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <BSplineLib/ParameterSpaces/knot_vector.hpp>

#include "splinepy/utils/print.hpp"

namespace splinepy::py {

namespace py = pybind11;

inline void add_knot_vector(py::module_& m) {
  using KnotVector = bsplinelib::parameter_spaces::KnotVector;
  using IterType = typename KnotVector::Knots_::iterator;
  using KnotType = typename KnotVector::Knots_::value_type;
  using DiffType = typename KnotVector::Knots_::difference_type;
  using SizeType = typename KnotVector::Knots_::size_type;

  // implementations adapted from pybind11/stl_bind.h

  auto wrap_id = [](DiffType i, const SizeType n) {
    if (i < 0) {
      i += n;
    }
    if (i < 0 || (SizeType) i >= n) {
      throw py::index_error();
    }
    return i;
  };
  py::class_<KnotVector, std::shared_ptr<KnotVector>> klasse(m, "KnotVector");
  klasse
      .def(
          "__iter__",
          [](KnotVector& kv) {
            return py::make_iterator<
                py::return_value_policy::reference_internal,
                IterType,
                IterType,
                KnotType&>(kv.GetKnots().begin(), kv.GetKnots().end());
          },
          py::keep_alive<0, 1>())
      .def(
          "__getitem__",
          [wrap_id](KnotVector& kv, DiffType i) -> KnotType& {
            i = wrap_id(i, kv.GetKnots().size());
            return kv.GetKnots()[static_cast<SizeType>(i)];
          },
          py::return_value_policy::reference_internal)
      .def(
          "__getitem__",
          [](const KnotVector& kv, const py::slice& slice) -> py::list {
            std::size_t start{}, stop{}, step{}, slicelength{};
            const auto kv_size = kv.GetKnots().size();
            if (!slice.compute(kv_size, &start, &stop, &step, &slicelength)) {
              throw py::error_already_set();
            }
            py::list items{};
            for (std::size_t i{}; i < slicelength; ++i) {
              items.append(kv[start]);
              start += step;
            }
            return items;
          },
          py::arg("slice"))
      .def("__setitem__",
           [wrap_id](KnotVector& kv, DiffType i, const KnotType knot) {
             i = wrap_id(i, kv.GetKnots().size());
             kv.UpdateKnot(i, knot);
           })
      .def("__setitem__",
           [](KnotVector& kv, const py::slice& slice, const py::list& value) {
             std::size_t start{}, stop{}, step{}, slicelength{};
             const auto kv_size = kv.GetKnots().size();
             if (!slice.compute(kv_size, &start, &stop, &step, &slicelength)) {
               throw py::error_already_set();
             }
             if (slicelength != value.size()) {
               splinepy::utils::PrintAndThrowError(
                   "Left and right hand size of slice assignment have "
                   "different sizes.");
             }
             auto& knots = kv.GetKnots();
             for (std::size_t i{}; i < slicelength; ++i) {
               knots[start] = py::cast<KnotType>(value[i]);
               start += step;
             }
             kv.ThrowIfTooSmallOrNotNonDecreasing();
           })
      .def("__repr__",
           [](const KnotVector& kv) {
             std::ostringstream s;
             s << "KnotVector [";
             int i{}, j{kv.GetSize() - 1};
             for (const auto& k : kv.GetKnots()) {
               s << k;
               if (i != j) {
                 s << ", ";
               }
               ++i;
             }
             s << "]";
             return s.str();
           })
      .def("scale", &KnotVector::Scale)
      .def("find_span", &KnotVector::FindSpan_)
      .def("numpy", [](const KnotVector& kv) {
        py::array_t<KnotType> arr(kv.GetSize());
        KnotType* arr_ptr = static_cast<KnotType*>(arr.request().ptr);
        for (int i{}; i < kv.GetSize(); ++i) {
          arr_ptr[i] = kv[i];
        }
        return arr;
      });
}

} // namespace splinepy::py
