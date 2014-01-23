/* 
   Copyright (c) Marshall Clow 2010-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.lslboost.org/LICENSE_1_0.txt)

    For more information, see http://www.lslboost.org
*/

#ifndef BOOST_ALGORITHM_BOYER_MOORE_HORSPOOOL_SEARCH_HPP
#define BOOST_ALGORITHM_BOYER_MOORE_HORSPOOOL_SEARCH_HPP

#include <iterator>     // for std::iterator_traits

#include <lslboost/assert.hpp>
#include <lslboost/static_assert.hpp>

#include <lslboost/range/begin.hpp>
#include <lslboost/range/end.hpp>

#include <lslboost/utility/enable_if.hpp>
#include <lslboost/type_traits/is_same.hpp>

#include <lslboost/algorithm/searching/detail/bm_traits.hpp>
#include <lslboost/algorithm/searching/detail/debugging.hpp>

// #define  BOOST_ALGORITHM_BOYER_MOORE_HORSPOOL_DEBUG_HPP

namespace lslboost { namespace algorithm {

/*
    A templated version of the boyer-moore-horspool searching algorithm.
    
    Requirements:
        * Random access iterators
        * The two iterator types (patIter and corpusIter) must 
            "point to" the same underlying type.
        * Additional requirements may be imposed buy the skip table, such as:
        ** Numeric type (array-based skip table)
        ** Hashable type (map-based skip table)

http://www-igm.univ-mlv.fr/%7Elecroq/string/node18.html

*/

    template <typename patIter, typename traits = detail::BM_traits<patIter> >
    class boyer_moore_horspool {
        typedef typename std::iterator_traits<patIter>::difference_type difference_type;
    public:
        boyer_moore_horspool ( patIter first, patIter last ) 
                : pat_first ( first ), pat_last ( last ),
                  k_pattern_length ( std::distance ( pat_first, pat_last )),
                  skip_ ( k_pattern_length, k_pattern_length ) {
                  
        //  Build the skip table
            std::size_t i = 0;
            if ( first != last )    // empty pattern?
                for ( patIter iter = first; iter != last-1; ++iter, ++i )
                    skip_.insert ( *iter, k_pattern_length - 1 - i );
#ifdef BOOST_ALGORITHM_BOYER_MOORE_HORSPOOL_DEBUG_HPP
            skip_.PrintSkipTable ();
#endif
            }
            
        ~boyer_moore_horspool () {}
        
        /// \fn operator ( corpusIter corpus_first, corpusIter corpus_last, Pred p )
        /// \brief Searches the corpus for the pattern that was passed into the constructor
        /// 
        /// \param corpus_first The start of the data to search (Random Access Iterator)
        /// \param corpus_last  One past the end of the data to search
        /// \param p            A predicate used for the search comparisons.
        ///
        template <typename corpusIter>
        corpusIter operator () ( corpusIter corpus_first, corpusIter corpus_last ) const {
            BOOST_STATIC_ASSERT (( lslboost::is_same<
                typename std::iterator_traits<patIter>::value_type, 
                typename std::iterator_traits<corpusIter>::value_type>::value ));

            if ( corpus_first == corpus_last ) return corpus_last;  // if nothing to search, we didn't find it!
            if (    pat_first ==    pat_last ) return corpus_first; // empty pattern matches at start

            const difference_type k_corpus_length  = std::distance ( corpus_first, corpus_last );
        //  If the pattern is larger than the corpus, we can't find it!
            if ( k_corpus_length < k_pattern_length )
                return corpus_last;
    
        //  Do the search 
            return this->do_search ( corpus_first, corpus_last );
            }
            
        template <typename Range>
        typename lslboost::range_iterator<Range>::type operator () ( Range &r ) const {
            return (*this) (lslboost::begin(r), lslboost::end(r));
            }

    private:
/// \cond DOXYGEN_HIDE
        patIter pat_first, pat_last;
        const difference_type k_pattern_length;
        typename traits::skip_table_t skip_;

        /// \fn do_search ( corpusIter corpus_first, corpusIter corpus_last )
        /// \brief Searches the corpus for the pattern that was passed into the constructor
        /// 
        /// \param corpus_first The start of the data to search (Random Access Iterator)
        /// \param corpus_last  One past the end of the data to search
        /// \param k_corpus_length The length of the corpus to search
        ///
        template <typename corpusIter>
        corpusIter do_search ( corpusIter corpus_first, corpusIter corpus_last ) const {
            corpusIter curPos = corpus_first;
            const corpusIter lastPos = corpus_last - k_pattern_length;
            while ( curPos <= lastPos ) {
            //  Do we match right where we are?
                std::size_t j = k_pattern_length - 1;
                while ( pat_first [j] == curPos [j] ) {
                //  We matched - we're done!
                    if ( j == 0 )
                        return curPos;
                    j--;
                    }
        
                curPos += skip_ [ curPos [ k_pattern_length - 1 ]];
                }
            
            return corpus_last;
            }
// \endcond
        };

/*  Two ranges as inputs gives us four possibilities; with 2,3,3,4 parameters
    Use a bit of TMP to disambiguate the 3-argument templates */

/// \fn boyer_moore_horspool_search ( corpusIter corpus_first, corpusIter corpus_last, 
///       patIter pat_first, patIter pat_last )
/// \brief Searches the corpus for the pattern.
/// 
/// \param corpus_first The start of the data to search (Random Access Iterator)
/// \param corpus_last  One past the end of the data to search
/// \param pat_first    The start of the pattern to search for (Random Access Iterator)
/// \param pat_last     One past the end of the data to search for
///
    template <typename patIter, typename corpusIter>
    corpusIter boyer_moore_horspool_search ( 
                  corpusIter corpus_first, corpusIter corpus_last, 
                  patIter pat_first, patIter pat_last )
    {
        boyer_moore_horspool<patIter> bmh ( pat_first, pat_last );
        return bmh ( corpus_first, corpus_last );
    }

    template <typename PatternRange, typename corpusIter>
    corpusIter boyer_moore_horspool_search ( 
        corpusIter corpus_first, corpusIter corpus_last, const PatternRange &pattern )
    {
        typedef typename lslboost::range_iterator<const PatternRange>::type pattern_iterator;
        boyer_moore_horspool<pattern_iterator> bmh ( lslboost::begin(pattern), lslboost::end (pattern));
        return bmh ( corpus_first, corpus_last );
    }
    
    template <typename patIter, typename CorpusRange>
    typename lslboost::lazy_disable_if_c<
        lslboost::is_same<CorpusRange, patIter>::value, typename lslboost::range_iterator<CorpusRange> >
    ::type
    boyer_moore_horspool_search ( CorpusRange &corpus, patIter pat_first, patIter pat_last )
    {
        boyer_moore_horspool<patIter> bmh ( pat_first, pat_last );
        return bm (lslboost::begin (corpus), lslboost::end (corpus));
    }
    
    template <typename PatternRange, typename CorpusRange>
    typename lslboost::range_iterator<CorpusRange>::type
    boyer_moore_horspool_search ( CorpusRange &corpus, const PatternRange &pattern )
    {
        typedef typename lslboost::range_iterator<const PatternRange>::type pattern_iterator;
        boyer_moore_horspool<pattern_iterator> bmh ( lslboost::begin(pattern), lslboost::end (pattern));
        return bmh (lslboost::begin (corpus), lslboost::end (corpus));
    }


    //  Creator functions -- take a pattern range, return an object
    template <typename Range>
    lslboost::algorithm::boyer_moore_horspool<typename lslboost::range_iterator<const Range>::type>
    make_boyer_moore_horspool ( const Range &r ) {
        return lslboost::algorithm::boyer_moore_horspool
            <typename lslboost::range_iterator<const Range>::type> (lslboost::begin(r), lslboost::end(r));
        }
    
    template <typename Range>
    lslboost::algorithm::boyer_moore_horspool<typename lslboost::range_iterator<Range>::type>
    make_boyer_moore_horspool ( Range &r ) {
        return lslboost::algorithm::boyer_moore_horspool
            <typename lslboost::range_iterator<Range>::type> (lslboost::begin(r), lslboost::end(r));
        }

}}

#endif  //  BOOST_ALGORITHM_BOYER_MOORE_HORSPOOOL_SEARCH_HPP
