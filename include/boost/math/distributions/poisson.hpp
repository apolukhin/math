// boost\math\distributions\poisson.hpp

// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2006.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Poisson distribution is a discrete probability distribution.
// It expresses the probability of a number (k) of
// events, occurrences, failures or arrivals occurring in a fixed time,
// assuming these events occur with a known average or mean rate (lambda)
// and are independent of the time since the last event.
// The distribution was discovered by Sim�on-Denis Poisson (1781�1840).

// Parameter lambda is the mean number of events in the given time interval.
// The random variate k is the number of events, occurrences or arrivals.
// k argument may be integral, signed, or unsigned, or floating point.
// If necessary, it has already been promoted from an integral type.

// Note that the Poisson distribution
// (like others including the binomial, negative binomial & Bernoulli)
// is strictly defined as a discrete function:
// only integral values of k are envisaged.
// However because the method of calculation uses a continuous gamma function,
// it is convenient to treat it as if a continous function,
// and permit non-integral values of k.
// To enforce the strict mathematical model, users should use floor or ceil functions
// on k outside this function to ensure that k is integral.

// See http://en.wikipedia.org/wiki/Poisson_distribution
// http://documents.wolfram.com/v5/Add-onsLinks/StandardPackages/Statistics/DiscreteDistributions.html

#ifndef BOOST_MATH_SPECIAL_POISSON_HPP
#define BOOST_MATH_SPECIAL_POISSON_HPP

#include <boost/math/special_functions/gamma.hpp> // for incomplete gamma. gamma_Q
#include <boost/math/distributions/complement.hpp> // complements
#include <boost/math/distributions/detail/common_error_handling.hpp> // error checks
#include <boost/math/special_functions/fpclassify.hpp> // isnan.
#include <boost/math/special_functions/factorials.hpp> // factorials.
#include <boost/math/tools/roots.hpp> // for root finding.

#include <utility>
using std::pair;

#if defined (BOOST_MSVC) && defined(BOOST_MATH_THROW_ON_DOMAIN_ERROR)
#  pragma warning(push)
#  pragma warning(disable: 4702) // unreachable code
// in domain_error_imp in error_handling
#  pragma warning(disable: 4189) // local variable is initialized but not referenced
#endif

namespace boost
{
  namespace math
  {
    namespace poisson_detail
    {
      // Common error checking routines for Poisson distribution functions.
      // These are convoluted, & apparently redundant, to try to ensure that
      // checks are always performed, even if exceptions are not enabled.

      template <class RealType>
      inline bool check_mean(const char* function, const RealType& mean, RealType* result)
      {
        if(!(boost::math::isfinite)(mean) || (mean < 0))
        {
          *result = tools::domain_error<RealType>(
            function,
            "Mean argument is %1%, but must be >= 0 !", mean);
          return false;
        }
        return true;
      } // bool check_mean

      template <class RealType>
      inline bool check_mean_NZ(const char* function, const RealType& mean, RealType* result)
      { // mean == 0 is considered an error.
        if( !(boost::math::isfinite)(mean) || (mean <= 0))
        {
          *result = tools::domain_error<RealType>(
            function,
            "Mean argument is %1%, but must be > 0 !", mean);
          return false;
        }
        return true;
      } // bool check_mean_NZ

      template <class RealType>
      inline bool check_dist(const char* function, const RealType& mean, RealType* result)
      { // Only one check, so this is redundant really but should be optimized away.
        return check_mean_NZ(function, mean, result);
      } // bool check_dist

      template <class RealType>
      inline bool check_k(const char* function, const RealType& k, RealType* result)
      {
        if((k < 0) || !(boost::math::isfinite)(k))
        {
          *result = tools::domain_error<RealType>(
            function,
            "Number of events k argument is %1%, but must be >= 0 !", k);
          return false;
        }
        return true;
      } // bool check_k

      template <class RealType>
      bool check_dist_and_k(const char* function, RealType mean, RealType k, RealType* result)
      {
        if((check_dist(function, mean, result) == false) ||
          (check_k(function, k, result) == false))
        {
          return false;
        }
        return true;
      } // bool check_dist_and_k

      template <class RealType>
      inline bool check_prob(const char* function, const RealType& p, RealType* result)
      { // Check 0 <= p <= 1
        if(!(boost::math::isfinite)(p) || (p < 0) || (p > 1))
        {
          *result = tools::domain_error<RealType>(
            function,
            "Probability argument is %1%, but must be >= 0 and <= 1 !", p);
          return false;
        }
        return true;
      } // bool check_prob

      template <class RealType>
      bool check_dist_and_prob(const char* function, RealType mean,  RealType p, RealType* result)
      {
        if((check_dist(function, mean, result) == false) ||
          (check_prob(function, p, result) == false))
        {
          return false;
        }
        return true;
      } // bool check_dist_and_prob

    } // namespace poisson_detail

    template <class RealType = double>
    class poisson_distribution
    {
    public:
      typedef RealType value_type;

      poisson_distribution(RealType mean = 1) : m_l(mean) // mean (lambda).
      { // Expected mean number of events that occur during the given interval.
        RealType r;
        poisson_detail::check_dist(
          BOOST_CURRENT_FUNCTION,
          m_l,
          &r);
      } // poisson_distribution constructor.

      RealType mean() const
      { // Private data getter function.
        return m_l;
      }

      // Parameter estimation:
      static RealType estimate_mean(RealType n, RealType k); // vector/array k TODO ????

    private:
      // Data member, initialized by constructor.
      RealType m_l; // mean number of occurrences.
    }; // template <class RealType> class poisson_distribution

    typedef poisson_distribution<double> poisson; // Reserved name of type double.

    // Non-member functions to give properties of the distribution.

    template <class RealType>
    const pair<RealType, RealType> range(const poisson_distribution<RealType>& dist)
    { // Range of permissible values for random variable k.
	    using boost::math::tools::max_value;
	    return const pair<RealType, RealType>(0, +max_value()); // Max integer?
    }

    template <class RealType>
    const pair<RealType, RealType> support(const poisson_distribution<RealType>& dist)
    { // Range of supported values for random variable k.
	    // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
	    using boost::math::tools::max_value;
	    return const pair<RealType, RealType>(0,  +max_value());
    }

    template <class RealType>
    inline RealType mean(const poisson_distribution<RealType>& dist)
    { // Mean of poisson distribution = lambda.
      return dist.mean();
    } // mean

    template <class RealType>
    inline RealType mode(const poisson_distribution<RealType>& dist)
    { // mode.
      return floor(dist.mean());
    }

    //template <class RealType>
    //inline RealType median(const poisson_distribution<RealType>& dist)
    //{ // median = approximately lambda + 1/3 - 0.2/lambda
    //  RealType l = dist.mean();
    //  return dist.mean() + static_cast<RealType>(0.3333333333333333333333333333333333333333333333)
    //   - static_cast<RealType>(0.2) / l;
    //} // BUT this formula appears to be out-by-one compared to quantile(half)
    // Query posted on Wikipedia.
    // Now implemented via quantile(half) in derived accessors.

    template <class RealType>
    inline RealType variance(const poisson_distribution<RealType>& dist)
    { // variance.
      return dist.mean();
    }

    // RealType standard_deviation(const poisson_distribution<RealType>& dist)
    // standard_deviation provided by derived accessors.

    template <class RealType>
    inline RealType skewness(const poisson_distribution<RealType>& dist)
    { // skewness = sqrt(l).
      return 1 / sqrt(dist.mean());
    }

    template <class RealType>
    inline RealType kurtosis_excess(const poisson_distribution<RealType>& dist)
    { // skewness = sqrt(l).
      return 1 / dist.mean(); // kurtosis_excess 1/mean from Wiki & MathWorld eq 31.
      // http://mathworld.wolfram.com/Kurtosis.html explains that the kurtosis excess
      // is more convenient because the kurtosis excess of a normal distribution is zero
      // whereas the true kurtosis is 3.
    } // RealType kurtosis_excess

    template <class RealType>
    inline RealType kurtosis(const poisson_distribution<RealType>& dist)
    { // kurtosis is 4th moment about the mean = u4 / sd ^ 4
      // http://en.wikipedia.org/wiki/Curtosis
      // kurtosis can range from -2 (flat top) to +infinity (sharp peak & heavy tails).
      // http://www.itl.nist.gov/div898/handbook/eda/section3/eda35b.htm
      return 3 + 1 / dist.mean(); // NIST.
      // http://mathworld.wolfram.com/Kurtosis.html explains that the kurtosis excess
      // is more convenient because the kurtosis excess of a normal distribution is zero
      // whereas the true kurtosis is 3.
    } // RealType kurtosis

    template <class RealType>
    RealType pdf(const poisson_distribution<RealType>& dist, const RealType k)
    { // Probability Density/Mass Function.
      // Probability that there are EXACTLY k occurrences (or arrivals).
      BOOST_FPU_EXCEPTION_GUARD

      using boost::math::tools::domain_error;
      using namespace std; // for ADL of std functions.

      RealType mean = dist.mean();
      // Error check:
      RealType result;
      if(false == poisson_detail::check_dist_and_k(
        BOOST_CURRENT_FUNCTION,
        mean,
        k,
        &result))
      {
        return result;
      }

      // Special case of mean zero, regardless of the number of events k.
      if (mean == 0)
      { // Probability for any k is zero.
        return 0;
      }
      if (k == 0)
      { // mean ^ k = 1, and k! = 1, so can simplify.
        return exp(-mean);
      }
      using boost::math::unchecked_factorial;
      RealType floork = floor(k);
      if ((floork == k) // integral
        && k < max_factorial<RealType>::value)
      { // k is small enough (for float 34, double 170 ...) to use factorial(k).
        return exp(-mean) * pow(mean, k) /
          unchecked_factorial<RealType>(tools::real_cast<unsigned int>(floork));
      }
      else
      { // Need to use log(factorial(k)) = lgamma(k+1)
        // (e ^ -mean * mean ^ k) / k!
        // == exp(log(e ^ -mean) + log (mean ^ k) - lgamma(k+1))
        // exp( -mean + log(mean) * k - lgamma(k+1))
        return exp(-mean + log(mean) * k - boost::math::lgamma(k+1));
        // return gamma_P_derivative(k+1, mean); // equivalent & also passes tests.
      }
    } // pdf

    template <class RealType>
    RealType cdf(const poisson_distribution<RealType>& dist, const RealType k)
    { // Cumulative Distribution Function Poisson.
      // The random variate k is the number of occurrences(or arrivals)
      // k argument may be integral, signed, or unsigned, or floating point.
      // If necessary, it has already been promoted from an integral type.
      // Returns the sum of the terms 0 through k of the Poisson Probability Density or Mass (pdf).

      // But note that the Poisson distribution
      // (like others including the binomial, negative binomial & Bernoulli)
      // is strictly defined as a discrete function: only integral values of k are envisaged.
      // However because of the method of calculation using a continuous gamma function,
      // it is convenient to treat it as if it is a continous function
      // and permit non-integral values of k.
      // To enforce the strict mathematical model, users should use floor or ceil functions
      // outside this function to ensure that k is integral.

      // The terms are not summed directly (at least for larger k)
      // instead the incomplete gamma integral is employed,

      using namespace std; // for ADL of std function exp.

      RealType mean = dist.mean();
      // Error checks:
      RealType result;
      if(false == poisson_detail::check_dist_and_k(
        BOOST_CURRENT_FUNCTION,
        mean,
        k,
        &result))
      {
        return result;
      }
      // Special cases:
      if (mean == 0)
      { // Probability for any k is zero.
        return 0;
      }
      if (k == 0)
      { // return pdf(dist, static_cast<RealType>(0));
        // but mean (and k) have already been checked,
        // so this avoids unnecessary repeated checks.
       return exp(-mean);
      }
      // For small integral k could use a finite sum -
      // it's cheaper than the gamma function.
      // BUT this is now done efficiently by gamma_Q function.
      // Calculate poisson cdf using the gamma_Q function.
      return gamma_Q(k+1, mean);
    } // binomial cdf

    template <class RealType>
    RealType cdf(const complemented2_type<poisson_distribution<RealType>, RealType>& c)
    { // Complemented Cumulative Distribution Function Poisson
      // The random variate k is the number of events, occurrences or arrivals.
      // k argument may be integral, signed, or unsigned, or floating point.
      // If necessary, it has already been promoted from an integral type.
      // But note that the Poisson distribution
      // (like others including the binomial, negative binomial & Bernoulli)
      // is strictly defined as a discrete function: only integral values of k are envisaged.
      // However because of the method of calculation using a continuous gamma function,
      // it is convenient to treat it as is it is a continous function
      // and permit non-integral values of k.
      // To enforce the strict mathematical model, users should use floor or ceil functions
      // outside this function to ensure that k is integral.

      // Returns the sum of the terms k+1 through inf of the Poisson Probability Density/Mass (pdf).
      // The terms are not summed directly (at least for larger k)
      // instead the incomplete gamma integral is employed,

      RealType const& k = c.param;
      poisson_distribution<RealType> const& dist = c.dist;

      RealType mean = dist.mean();

      // Error checks:
      RealType result;
      if(false == poisson_detail::check_dist_and_k(
        BOOST_CURRENT_FUNCTION,
        mean,
        k,
        &result))
      {
        return result;
      }
      // Special case of mean, regardless of the number of events k.
      if (mean == 0)
      { // Probability for any k is unity, complement of zero.
        return 1;
      }
      if (k == 0)
      { // Avoid repeated checks on k and mean in gamma_P.
       return -expm1(-mean);
      }
      // Unlike un-complemented cdf (sum from 0 to k),
      // can't use finite sum from k+1 to infinity for small integral k,
      // anyway it is now done efficiently by gamma_P.
      return gamma_P(k + 1, mean); // Calculate Poisson cdf using the gamma_P function.
      // CCDF = gamma_P(k+1, lambda)
    } // poisson ccdf

    template <class RealType>
    RealType quantile(const poisson_distribution<RealType>& dist, const RealType& p)
    { // Quantile (or Percent Point) Poisson function.
      // Return the number of expected events k for a given probability p.
      RealType result; // of Argument checks:
      if(false == poisson_detail::check_prob(
        BOOST_CURRENT_FUNCTION,
        p,
        &result))
      {
        return result;
      }
      // Special case:
      if (dist.mean() == 0)
      { // if mean = 0 then p = 0, so k can be anything?
         if (false == poisson_detail::check_mean_NZ(
         BOOST_CURRENT_FUNCTION,
         dist.mean(),
         &result))
        {
          return result;
        }
      }
      // if(p == 0) NOT necessarily zero!
      // Not necessarily any special value of k because is unlimited.
			if (p <= exp(-dist.mean()))
			{ // if p <= cdf for 0 events (== pdf for 0 events), then quantile must be zero.
				return 0;
			}
      return gamma_Q_inva(dist.mean(), p) - 1;
   } // quantile

    template <class RealType>
    RealType quantile(const complemented2_type<poisson_distribution<RealType>, RealType>& c)
    { // Quantile (or Percent Point) of Poisson function.
      // Return the number of expected events k for a given
      // complement of the probability q.
      //
      // Error checks:
      RealType q = c.param;
      const poisson_distribution<RealType>& dist = c.dist;
      RealType result;  // of argument checks.
      if(false == poisson_detail::check_prob(
        BOOST_CURRENT_FUNCTION,
        q,
        &result))
      {
        return result;
      }
      // Special case:
      if (dist.mean() == 0)
      { // if mean = 0 then p = 0, so k can be anything?
         if (false == poisson_detail::check_mean_NZ(
         BOOST_CURRENT_FUNCTION,
         dist.mean(),
         &result))
        {
          return result;
        }
      }
			if (-q <= expm1(-dist.mean()))
			{ // if q <= cdf(complement for 0 events, then quantile must be zero.
				return 0;
			}
      return gamma_P_inva(dist.mean(), q) -1;
   } // quantile complement.

  } // namespace math
} // namespace boost

#ifdef BOOST_MSVC
# pragma warning(pop)
#endif

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_MATH_SPECIAL_POISSON_HPP


