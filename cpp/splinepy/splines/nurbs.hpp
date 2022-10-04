#pragma once

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

// SplineLib
#include <Sources/Splines/nurbs.hpp>

#include <splinepy/proximity/proximity.hpp>
#include <splinepy/splines/helpers/extract.hpp>
#include <splinepy/splines/helpers/properties.hpp>
#include <splinepy/splines/helpers/scalar_type_wrapper.hpp>
#include <splinepy/splines/splinepy_base.hpp>

namespace splinepy::splines {

namespace py = pybind11;

template<int para_dim, int dim>
class Nurbs : public splinepy::splines::SplinepyBase,
              public splinelib::sources::splines::Nurbs<para_dim, dim> {
public:
  static constexpr int kParaDim = para_dim;
  static constexpr int kDim = dim;
  static constexpr bool kIsRational = true;

  // TODO rm after test
  constexpr static int para_dim_ = para_dim;
  constexpr static int dim_ = dim;

  // splinepy
  using SplinepyBase_ = typename splinepy::splines::SplinepyBase;

  // splinelib
  using Base_ = splinelib::sources::splines::Nurbs<para_dim, dim>;
  // Parameter space
  using ParameterSpace_ = typename Base_::ParameterSpace_;
  using Degrees_ = typename ParameterSpace_::Degrees_;
  using Degree_ = typename Degrees_::value_type;
  using KnotVectors_ = typename ParameterSpace_::KnotVectors_;
  using KnotVector_ = typename KnotVectors_::value_type::element_type;
  using Knots_ = typename Base_::Base_::Knots_;
  using Knot_ = typename Base_::Knot_;
  using KnotRatios_ = typename Base_::ParameterSpace_::KnotRatios_;
  using KnotRatio_ = typename KnotRatios_::value_type;
  using ParametricCoordinate_ = typename Base_::ParametricCoordinate_;
  using ScalarParametricCoordinate_ =
      typename ParametricCoordinate_::value_type;
  // Weighted Vector Space
  using WeightedVectorSpace_ = typename Base_::WeightedVectorSpace_;
  using Coordinates_ = typename WeightedVectorSpace_::Coordinates_;
  using Coordinate_ = typename Base_::Coordinate_;
  using ScalarCoordinate_ = typename Coordinate_::value_type;
  using Weights_ = typename WeightedVectorSpace_::Weights_;
  using Weight_ = typename Weights_::value_type;
  using VectorSpace_ = typename WeightedVectorSpace_::Base_; // <dim + 1>
  // Frequently used types
  using Derivative_ = typename Base_::Derivative_;
  using Dimension_ = typename splinelib::Dimension;
  using Tolerance_ = typename splinelib::sources::splines::Tolerance;
  using OutputInformation_ =
      splinelib::Tuple<typename ParameterSpace_::OutputInformation_,
                       typename WeightedVectorSpace_::OutputInformation_>;
  using Index_ = typename Base_::Base_::Index_;
  using IndexLength_ = typename Index_::Length_;
  using IndexValue_ = typename Index_::Value_;
  // Advanced use
  using HomogeneousBSpline_ = typename Base_::HomogeneousBSpline_;
  using Proximity_ = splinepy::proximity::Proximity<Nurbs<para_dim, dim>>;

  // raw ptr based inithelper.
  // degrees should have same size as parametric dimension
  // having knot_vectors vector of vector, we can keep track of their length,
  // as well as the legnth of control_points/weights.
  Base_ RawPtrInitHelper(const double* degrees,
                         const std::vector<std::vector<double>> knot_vectors,
                         const double* control_points,
                         const double* weights) {
    // process all the info and turn them into SplineLib types to initialize
    // Base_.

    // Prepare temporary containers
    Degrees_ sl_degrees;            // std::array
    Knots_ sl_knots;                // std::vector
    KnotVectors_ sl_knot_vectors;   // std::array
    Coordinates_ sl_control_points; // std::vector
    Weights_ sl_weights;            // std::vector
    std::size_t ncps{1};

    // Formulate degrees and knotvectors
    for (std::size_t i{}; i < kParaDim; ++i) {
      // degrees
      sl_degrees[i] = Degree_(degrees[i]);
      // knot vectors
      const auto& knot_vector = knot_vectors[i];
      std::size_t nkv = knot_vector.size();
      sl_knots.clear();
      sl_knots.reserve(nkv);
      // try if this works after namedtype ext
      // sl_knots = knot_vector;
      for (std::size_t j{}; j < nkv; ++j) {
        sl_knots.emplace_back(Knot_{knot_vector[j]});
      }
      std::shared_ptr sl_knot_vector{std::make_shared<KnotVector_>(sl_knots)};
      sl_knot_vectors[i] = sl_knot_vector;

      ncps *= nkv - degrees[i] - 1;
    }

    // Formulate ParameterSpace
    auto sl_parameter_space =
        std::make_shared<ParameterSpace_>(sl_knot_vectors, sl_degrees);

    // Formulate control_points and weights
    sl_control_points.reserve(ncps);
    sl_weights.reserve(ncps);
    for (std::size_t i{}; i < ncps; ++i) {
      // control_points
      Coordinate_ control_point;
      for (std::size_t j{}; j < kDim; ++j) {
        control_point[j] = ScalarCoordinate_{control_points[i * kDim + j]};
      }
      sl_control_points.push_back(std::move(control_point));

      // weights
      sl_weights.emplace_back(weights[i]);
    }

    auto sl_weighted_space =
        std::make_shared<WeightedVectorSpace_>(sl_control_points, sl_weights);

    // return init
    return Base_(sl_parameter_space, sl_weighted_space);
  }

  // rawptr based ctor
  Nurbs(const double* degrees,
        const std::vector<std::vector<double>>& knot_vectors,
        const double* control_points,
        const double* weights)
      : Base_(
          RawPtrInitHelper(degrees, knot_vectors, control_points, weights)) {}
  // inherit ctor
  using Base_::Base_;

  const Degrees_& GetDegrees() const {
    return GetParameterSpace().GetDegrees();
  };

  // required implementations
  virtual constexpr int SplinepyParaDim() const { return kParaDim; }

  virtual constexpr int SplinepyDim() const { return kDim; }

  virtual std::string SplinepySplineName() const { return "NURBS"; }

  virtual std::string SplinepyWhatAmI() const {
    return "NURBS, parametric dimension: " + std::to_string(SplinepyParaDim())
           + ", physical dimension: " + std::to_string(SplinepyDim());
  }

  virtual bool SplinepyHasKnotVectors() const { return true; }

  virtual bool SplinepyIsRational() const { return true; }

  virtual int SplinepyNumberOfControlPoints() const {
    return GetWeightedVectorSpace().GetNumberOfCoordinates();
  }

  virtual int SplinepyNumberOfSupports() const {
    return splinepy::splines::helpers::GetNumberOfSupports(*this);
  }

  virtual void
  SplinepyCurrentProperties(double* degrees,
                            std::vector<std::vector<double>>* knot_vectors,
                            double* control_points,
                            double* weights) const {
    const auto& parameter_space = GetParameterSpace();
    const auto& vector_space = GetWeightedVectorSpace();

    // degrees
    for (std::size_t i{}; i < kParaDim; ++i) {
      degrees[i] = static_cast<double>(parameter_space.GetDegrees()[i]);
    }

    // knot_vectors
    const auto& core_kvs = parameter_space.GetKnotVectors();
    knot_vectors->clear();
    knot_vectors->reserve(kParaDim);
    for (std::size_t i{}; i < kParaDim; ++i) {
      const auto& core_kv = *core_kvs[i];
      const std::size_t kvsize = static_cast<std::size_t>(core_kv.GetSize());
      std::vector<double> kv;
      kv.reserve(kvsize);
      // Index wants int
      for (int j{}; j < kvsize; ++j) {
        kv.emplace_back(static_cast<double>(core_kv[splinelib::Index{j}]));
      }
      knot_vectors->push_back(std::move(kv));
    }

    // control_points and weights
    std::size_t ncps = vector_space.GetNumberOfCoordinates();
    // fill it up, phil!
    for (std::size_t i{}; i < ncps; ++i) {
      auto const& coord_named_phil = vector_space[splinelib::Index{
          static_cast<splinelib::Index::Type_>(i)}];
      // unweight - phil needs to first project before filling.
      auto const& projected_phil =
          WeightedVectorSpace_::Project(coord_named_phil);
      for (std::size_t j{}; j < kDim; ++j) {
        control_points[i * kDim + j] = static_cast<double>(projected_phil[j]);
      }
      weights[i] = static_cast<double>(coord_named_phil[dim]);
    }
  }

  virtual void SplinepyParametricBounds(double* para_bounds) const {
    const auto pbounds = splinepy::splines::helpers::GetParametricBounds(*this);
    for (std::size_t i{}; i < kParaDim; ++i) {
      // lower bounds
      para_bounds[i] = pbounds[0][i];
      // upper bounds
      para_bounds[kParaDim + i] = pbounds[1][i];
    }
  }

  virtual void SplinepyEvaluate(const double* para_coord,
                                double* evaluated) const {
    splinepy::splines::helpers::ScalarTypeEvaluate(*this,
                                                   para_coord,
                                                   evaluated);
  }

  virtual void SplinepyDerivative(const double* para_coord,
                                  const int* orders,
                                  double* derived) const {
    splinepy::splines::helpers::ScalarTypeDerivative(*this,
                                                     para_coord,
                                                     orders,
                                                     derived);
  }

  virtual void SplinepyBasisAndSupport(const double* para_coord,
                                       double* basis,
                                       int* support) const {
    ParameterSpace_ const& parameter_space = *Base_::Base_::parameter_space_;

    typename ParameterSpace_::UniqueEvaluations_ unique_evaluations;
    parameter_space.template InitializeUniqueEvaluations<false>(
        unique_evaluations);

    ParametricCoordinate_ sl_para_coord;
    for (std::size_t i{}; i < kParaDim; ++i) {
      sl_para_coord[i] = ScalarParametricCoordinate_{para_coord[i]};
    }

    int i{0};
    double W{0.};
    for (Index_ non_zero_basis_function{parameter_space.First()};
         non_zero_basis_function != parameter_space.Behind();
         ++non_zero_basis_function) {

      Index_ const& basis_function =
          (parameter_space.FindFirstNonZeroBasisFunction(sl_para_coord)
           + non_zero_basis_function.GetIndex());

      // general basis fn
      const auto evaluated =
          parameter_space.EvaluateBasisFunction(basis_function,
                                                non_zero_basis_function,
                                                sl_para_coord,
                                                unique_evaluations);

      // get `w` and add to `W`
      const auto& support_id = basis_function.GetIndex1d(); // not int yet

      VectorSpace_ const& vector_space = *Base_::weighted_vector_space_;
      const auto& w = vector_space[support_id][dim].Get(); // dim: last elem

      const double N_times_w = evaluated * w;

      W += N_times_w;
      basis[i] = N_times_w; // not yet final
      support[i] = support_id.Get();
      ++i;
    }

    // Loop and divide entries by W
    int end = i;
    double W_inv = 1 / W;
    for (i = 0; i < end; ++i) {
      basis[i] *= W_inv;
    }
  }

  virtual void SplinepyElevateDegree(const int& p_dim) {
    splinepy::splines::helpers::ScalarTypeElevateDegree(*this, p_dim);
  }

  virtual bool SplinepyReduceDegree(const int& p_dim, const double& tolerance) {
    return splinepy::splines::helpers::ScalarTypeReduceDegree(*this,
                                                              p_dim,
                                                              tolerance);
  }

  virtual void SplinepyInsertKnot(const int& p_dim, const double& knot) {
    splinepy::splines::helpers::ScalarTypeInsertKnot(*this, p_dim, knot);
  }

  virtual bool SplinepyRemoveKnot(const int& p_dim,
                                  const double& knot,
                                  const double& tolerance) {
    return splinepy::splines::helpers::ScalarTypeRemoveKnot(*this,
                                                            p_dim,
                                                            knot,
                                                            tolerance);
  }

  /// Bezier patch extraction
  virtual std::vector<std::shared_ptr<SplinepyBase>>
  SplinepyExtractBezierPatches() const {
    return splinepy::splines::helpers::ExtractBezierPatches<true>(*this);
  }

  const ParameterSpace_& GetParameterSpace() const {
    return *Base_::Base_::parameter_space_;
  }

  const WeightedVectorSpace_& GetWeightedVectorSpace() const {
    return *Base_::weighted_vector_space_;
  }

  /*
   * Update degrees since its size never changes
   *
   * Parameters
   * -----------
   * ds_buf_ptr: out
   */
  void UpdateDegrees(int* ds_buf_ptr) {

    ParameterSpace_ const& parameter_space = *Base_::Base_::parameter_space_;
    for (int i = 0; i < para_dim; i++) {
      ds_buf_ptr[i] = parameter_space.GetDegrees()[i].Get();
    }
  }

  /*
   * Update current knot vectors to python
   * since list is mutable, update works
   *
   * Parameters
   * -----------
   * p_knot_vectors: inout
   *  will be `clear`ed before writing.
   */
  void UpdateKnotVectors(py::list& p_knot_vectors) {

    // start clean
    p_knot_vectors.attr("clear")();

    ParameterSpace_ const& parameter_space = *Base_::Base_::parameter_space_;
    for (auto& knotvector : parameter_space.GetKnotVectors()) {
      auto const& kv = *knotvector; // in
      py::list p_kv;                // out
      for (int i = 0; i < kv.GetSize(); i++) {
        auto const& knot = kv[splinelib::Index{i}];
        p_kv.append(knot.Get());
      }
      p_knot_vectors.append(p_kv);
    }
  }

  /* Returns n_cpts
   * used to allocate numpy array
   *
   * Parameters
   * -----------
   * None
   *
   * Returns
   * -------
   * N_CPS
   */
  int GetNCps() {
    WeightedVectorSpace_ const& vector_space = *Base_::weighted_vector_space_;
    return vector_space.GetNumberOfCoordinates();
  }

  /* Update cps and weights at the same time since they belong together in
   * weightedvectorspace
   *
   * Parameters
   * -----------
   * cps_buf_ptr: out
   *   size -> (n_cps * dim)
   * ws_buf_ptr: out
   *   size -> (n_ws) (=n_cps)
   */
  void UpdateControlPointsAndWeights(double* cps_buf_ptr, double* ws_buf_ptr) {
    WeightedVectorSpace_ const& vector_space = *Base_::weighted_vector_space_;
    int numcps = vector_space.GetNumberOfCoordinates();

    // fill it up, phil!
    for (int i = 0; i < numcps; i++) {
      auto const& coord_named_phil = vector_space[splinelib::Index{i}];
      // phil needs to first project before filling.
      auto const& projected_phil =
          WeightedVectorSpace_::Project(coord_named_phil);

      for (int j = 0; j < dim; j++) {
        cps_buf_ptr[i * dim + j] = projected_phil[j].Get();
      }
      ws_buf_ptr[i] = coord_named_phil[dim].Get();
    }
  }

  /* Given parametric coordinate, returns basis functions and support control
   * point ids.
   * pointer should have size of (degree + 1)^(para_dim)
   *
   * Parameters
   * -----------
   * parametric_coordinate: in
   * basis_function_values: out
   * support_control_point_ids: out
   */
  void BasisFunctionsAndIDs(ParametricCoordinate_ const& parametric_coordinate,
                            double* basis_function_values,
                            int* support_control_point_ids) const {

    ParameterSpace_ const& parameter_space = *Base_::Base_::parameter_space_;

    int i = 0;
    double W = 0.0;
    for (Index_ non_zero_basis_function{parameter_space.First()};
         non_zero_basis_function != parameter_space.Behind();
         ++non_zero_basis_function) {

      Index_ const& basis_function =
          (parameter_space.FindFirstNonZeroBasisFunction(parametric_coordinate)
           + non_zero_basis_function.GetIndex());

      // general basis fn
      const auto evaluated =
          parameter_space.EvaluateBasisFunction(basis_function,
                                                parametric_coordinate);

      // get `w` and add to `W`
      const auto& support_id = basis_function.GetIndex1d(); // not int yet

      VectorSpace_ const& vector_space = *Base_::weighted_vector_space_;
      const auto& w = vector_space[support_id][dim].Get(); // dim: last elem

      const double N_times_w = evaluated * w;

      W += N_times_w;
      basis_function_values[i] = N_times_w; // not yet final
      support_control_point_ids[i] = support_id.Get();
      i++;
    }

    // Loop and divide entries by W
    int end = i;
    double W_inv = 1 / W;
    for (i = 0; i < end; i++) {
      basis_function_values[i] *= W_inv;
    }
  }

  /* Fn overload with pure array in,outs.
   *
   * Parameters
   * -----------
   * parametric_coordinate: in
   * basis_function_values: out
   * support_control_point_ids: out
   */
  void BasisFunctionsAndIDs(const double* parametric_coordinate,
                            double* basis_function_values,
                            int* support_control_point_ids) const {
    // prepare parametric coordinate
    ParametricCoordinate_ pc{};
    for (int i{0}; i < para_dim; i++) {
      pc[i] = ScalarParametricCoordinate_{parametric_coordinate[i]};
    }

    BasisFunctionsAndIDs(parametric_coordinate,
                         basis_function_values,
                         support_control_point_ids);
  }

  Proximity_& GetProximity() {
    if (!proximity_initialized_) {
      proximity_ = std::make_unique<Proximity_>(*this);
      proximity_initialized_ = true;
    }
    return *proximity_;
  }

protected:
  std::unique_ptr<Proximity_> proximity_;
  bool proximity_initialized_ = false;

}; /* class Nurbs */

std::shared_ptr<SplinepyBase> SplinepyBase::SplinepyCreateNurbs(
    const int para_dim,
    const int dim,
    const double* degrees,
    const std::vector<std::vector<double>>* knot_vectors,
    const double* control_points,
    const double* weights) {
  switch (para_dim) {
  case 1:
    switch (dim) {
    case 1:
      return std::shared_ptr<SplinepyBase>(
          new splinepy::splines::Nurbs<1, 1>(degrees,
                                             *knot_vectors,
                                             control_points,
                                             weights));
    case 2:
      return std::make_shared<Nurbs<1, 2>>(degrees,
                                           *knot_vectors,
                                           control_points,
                                           weights);
    }
  case 2:
    switch (dim) {
    case 1:
      return std::make_shared<Nurbs<2, 1>>(degrees,
                                           *knot_vectors,
                                           control_points,
                                           weights);

    case 2:
      return std::make_shared<Nurbs<2, 2>>(degrees,
                                           *knot_vectors,
                                           control_points,
                                           weights);
    }
  }
}

} /* namespace splinepy::splines */
