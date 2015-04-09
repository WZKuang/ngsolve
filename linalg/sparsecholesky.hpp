#ifndef FILE_SPARSECHOLESKY
#define FILE_SPARSECHOLESKY

/* *************************************************************************/
/* File:   sparsecholesky.hpp                                              */
/* Author: Joachim Schoeberl                                               */
/* Date:   18. Jun. 97                                                     */
/* *************************************************************************/

/*
  sparse cholesky factorization
*/

namespace ngla
{

  class SparseFactorization : public BaseMatrix
  { 
  protected:
    const BaseSparseMatrix & matrix;
    const BitArray * inner;
    const Array<int> * cluster;
    bool smooth_is_projection;

  public:
    SparseFactorization (const BaseSparseMatrix & amatrix,
			 const BitArray * ainner,
			 const Array<int> * acluster);
 
    virtual void Smooth (BaseVector & u, const BaseVector & f, BaseVector & y) const;

    int VHeight(){ return matrix.VWidth();}
    int VWidth(){ return matrix.VHeight();}

    bool SmoothIsProjection () const { return smooth_is_projection; }
  };


  /**
     A sparse cholesky factorization.
     The unknowns are reordered by the minimum degree
     ordering algorithm
  */

  template<class TM>
	   // class TV_ROW = typename mat_traits<TM>::TV_ROW, 
	   // class TV_COL = typename mat_traits<TM>::TV_COL>
  class SparseCholeskyTM : public SparseFactorization
  {
  protected:
    int height, nze;

    Array<int, size_t> order, firstinrow, firstinrow_ri, rowindex2;

    Array<int> blocknrs;
    Array<int> blocks; // block nr. i has dofs  [blocks[i], blocks[i+1])
    Table<int> block_dependency; 

    class MicroTask
    {
    public:
      int blocknr;
      bool solveL;
      int bblock;
      int nbblocks;
    };
    
    Array<MicroTask> microtasks;
    Table<int> micro_dependency;     
    Table<int> micro_dependency_trans;     


    Array<TM, size_t> lfact;
    Array<TM, size_t> diag;

    ///
    MinimumDegreeOrdering * mdo;
    int maxrow;
    const SparseMatrixTM<TM> & mat;

  public:
    typedef typename mat_traits<TM>::TSCAL TSCAL_MAT;

    ///
    SparseCholeskyTM (const SparseMatrixTM<TM> & a, 
                      const BitArray * ainner = NULL,
                      const Array<int> * acluster = NULL,
                      bool allow_refactor = 0);
    ///
    virtual ~SparseCholeskyTM ();
    ///
    int VHeight() const { return height; }
    ///
    int VWidth() const { return height; }
    ///
    void Allocate (const Array<int> & aorder, 
		   const Array<MDOVertex> & vertices,
		   const int * blocknr);
    ///
    void Factor (); 
#ifdef LAPACK
    void FactorSPD (); 
#endif
    ///
    void FactorNew (const SparseMatrix<TM> & a);

    /**
       A = L+D+L^T
       y = f - (L+D)^T u
       w = C^{-1} (y - L u)
       u += w
       y -= (L+D)^T w
    **/
    // virtual void Smooth (BaseVector & u, const BaseVector & f, BaseVector & y) const;
    ///
    virtual ostream & Print (ostream & ost) const;

    virtual void MemoryUsage (Array<MemoryUsageStruct*> & mu) const
    {
      mu.Append (new MemoryUsageStruct ("SparseChol", nze*sizeof(TM), 1));
    }


    ///
    void Set (int i, int j, const TM & val);
    ///
    const TM & Get (int i, int j) const;
    ///
    void SetOrig (int i, int j, const TM & val)
    { Set (order[i], order[j], val); }


    IntRange BlockDofs (int bnr) const { return Range(blocks[bnr], blocks[bnr+1]); }

    FlatArray<int> BlockExtDofs (int bnr) const
    {
      auto range = BlockDofs (bnr);
      int base = firstinrow_ri[range.begin()] + range.Size()-1;
      int ext_size =  firstinrow[range.begin()+1]-firstinrow[range.begin()] - range.Size()+1;
      return rowindex2.Range(base, base+ext_size);
    }
  };









  template<class TM, 
	   class TV_ROW = typename mat_traits<TM>::TV_ROW, 
	   class TV_COL = typename mat_traits<TM>::TV_COL>
  class SparseCholesky : public SparseCholeskyTM<TM>
  {
    typedef SparseCholeskyTM<TM> BASE;
    using BASE::height;
    using BASE::Height;
    using BASE::inner;
    using BASE::cluster;

    using BASE::lfact;
    using BASE::diag;
    using BASE::order;
    using BASE::firstinrow;

    using BASE::MicroTask;
    using BASE::microtasks;
    using BASE::micro_dependency;
    using BASE::micro_dependency_trans;
    using BASE::BlockDofs;
    using BASE::BlockExtDofs;
  public:
    typedef TV_COL TV;
    typedef TV_ROW TVX;
    typedef typename mat_traits<TV_ROW>::TSCAL TSCAL_VEC;

    
    SparseCholesky (const SparseMatrixTM<TM> & a, 
		    const BitArray * ainner = NULL,
		    const Array<int> * acluster = NULL,
		    bool allow_refactor = 0)
      : SparseCholeskyTM<TM> (a, ainner, acluster, allow_refactor) { ; }

    ///
    virtual ~SparseCholesky () { ; }
    
    virtual void Mult (const BaseVector & x, BaseVector & y) const;

    virtual void MultAdd (TSCAL_VEC s, const BaseVector & x, BaseVector & y) const;

    virtual AutoVector CreateVector () const
    {
      return make_shared<VVector<TV>> (height);
    }


    void SolveBlock (int i, FlatVector<> hy) const;
    void SolveBlockT (int i, FlatVector<> hy) const;
  };


}

#endif
