/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file	zypp/ui/Selectable.h
 *
*/
#ifndef ZYPP_UI_SELECTABLE_H
#define ZYPP_UI_SELECTABLE_H

#include <iosfwd>

#include "zypp/base/ReferenceCounted.h"
#include "zypp/base/NonCopyable.h"
#include "zypp/base/PtrTypes.h"
#include "zypp/base/Iterator.h"

#include "zypp/ui/SelectableTraits.h"
#include "zypp/ui/Status.h"

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////
  namespace ui
  { /////////////////////////////////////////////////////////////////

    DEFINE_PTR_TYPE(Selectable);

    ///////////////////////////////////////////////////////////////////
    //
    //	CLASS NAME : Selectable
    //
    /** Collects PoolItems of same kind and name.
     *
     * Selectable is a status wrapper. The ui::Status is calculated
     * from (and transated to) \ref PoolItems individual \ref ResStatus
     * values.
     *
     * Available objects are sorted according the solver policies, 'best'
     * packages first (e.g. by repository priority, then Arch, then Edition).
     *
     * Installed objects are sorted according the installation date, newer install
     * time first.
    */
    class Selectable : public base::ReferenceCounted, private base::NonCopyable
    {
      friend std::ostream & operator<<( std::ostream & str, const Selectable & obj );
      friend std::ostream & dumpOn( std::ostream & str, const Selectable & obj );

    public:
      typedef intrusive_ptr<Selectable>        Ptr;
      typedef intrusive_ptr<const Selectable>  constPtr;

      /** Iterates over ResObject::constPtr */
      typedef SelectableTraits::available_iterator      available_iterator;
      typedef SelectableTraits::available_size_type     available_size_type;

      typedef SelectableTraits::installed_iterator      installed_iterator;
      typedef SelectableTraits::installed_size_type     installed_size_type;

    public:
      /** \name Static ctor substitues picking the item from the pool.
       * \code
       * Selectable::Ptr item;
       * item = Selectable::get( "amarok );                  // package amamrok
       * item = Selectable::get( ResKind::patch, "amarok );  // patch amamrok
       * item = Selectable::get( IdString( "patch:amarok" ); // patch amamrok
       * \endcode
      */
      //@{
      /** Get the \ref Selctable */
      static Ptr get( const pool::ByIdent & ident_r );

      /** Get the \ref Selctable by it's \c sat-identifyer. */
      static Ptr get( IdString ident_r )
      { return get( pool::ByIdent( ident_r ) ); }

      /** Get the \ref Selctable by \c kind and \c name. */
      static Ptr get( ResKind kind_r, const std::string & name_r )
      { return get( pool::ByIdent( kind_r, name_r ) ); }

      /** Get the \c Package \ref Selctable by \c name. */
      static Ptr get( const std::string & name_r )
      { return get( pool::ByIdent( ResKind::package, name_r ) ); }

      /** Get the \ref Selctable containing a specific \ref sat::Solvable. */
      static Ptr get( const sat::Solvable & solv_r )
      { return get( pool::ByIdent( solv_r ) ); }

      /** Get the \ref Selctable containing a specific \ref ResObject. */
      static Ptr get( const ResObject::constPtr & resolvable_r )
      { return resolvable_r ? get( resolvable_r->satSolvable() ) : Ptr(); }

      /** Get the \ref Selctable containing a specific \ref PoolItem. */
      static Ptr get( const PoolItem & pi_r )
      { return get( pi_r.satSolvable() ); }
      //@}

    public:
      /** The identifier.
       * This is the solvables \ref name, \b except for packages and
       * source packes, prefixed by it's \ref kind.
       * \see \ref sat::Solvable.
       */
      IdString ident() const;

      /** The ResObjects kind. */
      ResObject::Kind kind() const;

      /** The ResObjects name.  */
      const std::string & name() const;

      /** The last Installed object. */
      PoolItem installedObj() const;

      /**
       * Returns one of available objects, specifically either the user
       * selected candidate or a default.
       *
       * Default is either the first available object which has the same arch
       * as one of the installed objects, or the first available object
       * (if none of available arch matches the arch of the installed objects),
       * or empty.
       *
       * \return a PoolItem according to the describe criteria.
       */
      PoolItem candidateObj() const;

      /** Return the \ref installedObj resolvable casted to a specific kind.
       * \code
       *   Selectable mySelectable;
       *   Package::constPtr p( mySelectable.installedAsKind<Package>() );
       * \endcode
      */
      template<class _Res>
      typename ResTraits<_Res>::constPtrType installedAsKind() const
      { return asKind<_Res>( candidateObj() ); }

      /** Return the \ref candidateObj resolvable casted to a specific kind.
       * \code
       *   Selectable mySelectable;
       *   Package::constPtr p( mySelectable.candidateAsKind<Package>() );
       * \endcode
      */
      template<class _Res>
      typename ResTraits<_Res>::constPtrType candidateAsKind() const
      { return asKind<_Res>( candidateObj() ); }

      /** Set a candidate (out of available objects).
       * \return The new candidate, or NULL if choice was invalid
       * (NULL or not among availableObjs). An invalid choice
       * selects the default candidate.
       * In case the causer is not \c ResStatus::USER the operation
       * may also fail if there are insufficient permissions to change
       * a transacting candidate.
       */
      PoolItem setCandidate( const PoolItem & newCandidate_r, ResStatus::TransactByValue causer_r = ResStatus::USER );
      /** \overload */
      PoolItem setCandidate( ResObject::constPtr newCandidate_r, ResStatus::TransactByValue causer_r = ResStatus::USER );

      /**
       * The object whose summary to show in the UIs.
       *
       * \return the \ref candidateObj(), if not empty, or the first of installed
       *         objects or an empty \ref PoolItem.
       * \see candidateObj()
       */
      PoolItem theObj() const;

      ////////////////////////////////////////////////////////////////////////

      /** \name Available objects iterators.
       * Oredered according to solver policy. 'Best' first.
      */
      //@{
      bool availableEmpty() const;
      available_size_type availableSize() const;
      available_iterator availableBegin() const;
      available_iterator availableEnd() const;
      //@}

      ////////////////////////////////////////////////////////////////////////

      /** \name Insatlled objects iterators.
       * Ordered by install time. Latest first.
      */
      //@{
      bool installedEmpty() const;
      installed_size_type installedSize() const;
      installed_iterator installedBegin() const;
      installed_iterator installedEnd() const;
      //}

      ////////////////////////////////////////////////////////////////////////

    public:
      /** \name Query for objects within this Selectable.
      */
      //@{
      /** True if either installed or candidate object is present */
      bool hasObject() const
      { return (! installedEmpty()) || candidateObj(); }

      /** True if installed object is present. */
      bool hasInstalledObj() const
      { return ! installedEmpty(); }

      /** True if candidate object is present. */
      bool hasCandidateObj() const
      { return candidateObj(); }

      /** True if installed and candidate object is present */
      bool hasBothObjects() const
      { return (! installedEmpty()) && candidateObj(); }

      /** True if installed object is present but no candidate. */
      bool hasInstalledObjOnly() const
      { return (! installedEmpty()) && ! candidateObj(); }

      /** True if candidate object is present but no installed. */
      bool hasCandidateObjOnly() const
      { return ( installedEmpty() ) && candidateObj(); }
      //@}

      /**
       * True if this package has no replacement from
       * the available repositories
       */
      bool isUnmaintained() const;

      /** \name Classification of available patches (pseudo installed items).
       * A patch is either \c not \c relevant, \c satisfied or \c broken.
       * The same applies to other pseudo installed kinds.
       * \see \ref traits::isPseudoInstalled
       */
      //@{
      /** Returns true for packages, because packages are not
       * classified by the solver.
      */
      bool isUndetermined() const;

      /** Returns true if the patch is relevant which means that at least
       *  one package of the patch is installed.
       */
      bool isRelevant() const;

      /** Whether a relevant patchs requirements are met. */
      bool isSatisfied() const;

      /** Whether a relevant patchs requirements are broken. */
      bool isBroken() const;

      /** This includes still broken patches, as well as those already
       *  selected to be installed.
       * This is because already selected patches will be classified as
       * \c satisfied.
       */
      bool isNeeded() const;
      //@}

     public:
      /** \name Query and maip objects fate in case of commit.
      */
      //@{
      enum Fate {
        TO_DELETE  = -1,
        UNMODIFIED = 0,
        TO_INSTALL = 1
      };
      /**  */
      Fate fate() const;

      /** True if either to delete or to install */
      bool unmodified() const
      { return fate() == UNMODIFIED; }

      /** True if either to delete or to install */
      bool toModify() const
      { return fate() != UNMODIFIED; }

      /** True if to delete */
      bool toDelete() const
      { return fate() == TO_DELETE; }

      /** True if to install */
      bool toInstall() const
      { return fate() == TO_INSTALL; }

      /** */
      bool setFate( Fate fate_r, ResStatus::TransactByValue causer_r = ResStatus::USER );

      /** Set the item to be installed (new- or re-install). */
      bool setToInstall( ResStatus::TransactByValue causer_r = ResStatus::USER )
      { return setFate( TO_INSTALL, causer_r ); }

      /** Take care the item gets installed if it is not. */
      bool setInstalled( ResStatus::TransactByValue causer_r = ResStatus::USER );

      /** Take care the item gets installed if it is not, or is older. */
      bool setUpToDate( ResStatus::TransactByValue causer_r = ResStatus::USER );

      /** Set the item to be deleted (must be installed). */
      bool setToDelete( ResStatus::TransactByValue causer_r = ResStatus::USER )
      { return setFate( TO_DELETE, causer_r ); }

      /** Take care the item gets deleted if it is installed. */
      bool setDeleted( ResStatus::TransactByValue causer_r = ResStatus::USER );

      /** Set the item to stay unmodified. */
      bool unset( ResStatus::TransactByValue causer_r = ResStatus::USER )
      { return setFate( UNMODIFIED, causer_r ); }
      //@}

    public:
      /**
       * \name Special inteface for Y2UI.
       * \note This interface acts on \ref ResStatus::USER level.
       * The \ref Status enum, and allowed state transitions are
       * tightly related to the Y2UI.
      */
      //@{
      /** Return the current Status */
      Status status() const;

      /**
       * Try to set a new Status.
       * Returns \c false if the transitions is not allowed.
       */
      bool setStatus( const Status state_r, ResStatus::TransactByValue causer_r = ResStatus::USER );

      /** Return who caused the modification. */
      ResStatus::TransactByValue modifiedBy() const;

      /** Return value of LicenceConfirmed bit. */
      bool hasLicenceConfirmed() const;

      /** Set LicenceConfirmed bit. */
      void setLicenceConfirmed( bool val_r = true );
      //@}

    public:
      /** Implementation  */
      class Impl;
      typedef shared_ptr<Impl> Impl_Ptr;
      /** Default ctor */
      Selectable( Impl_Ptr pimpl_r );
    private:
      /** Dtor */
      ~Selectable();
    private:
      /** Pointer to implementation */
      RW_pointer<Impl> _pimpl;
    };
    ///////////////////////////////////////////////////////////////////

    /** \relates Selectable Stream output */
    std::ostream & operator<<( std::ostream & str, const Selectable & obj );

    /** \relates Selectable More verbose stream output */
    std::ostream & dumpOn( std::ostream & str, const Selectable & obj );

    /** Solvable to Selectable transform functor.
     * \relates Selectable
     * \relates sat::SolvIterMixin
     */
    struct asSelectable
    {
      typedef Selectable_Ptr result_type;

      Selectable_Ptr operator()( const sat::Solvable & solv_r ) const;

      Selectable_Ptr operator()( const PoolItem & pi_r ) const
      { return operator()( pi_r.satSolvable() ); }
    };

    /////////////////////////////////////////////////////////////////
  } // namespace ui
  ///////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
#endif // ZYPP_UI_SELECTABLE_H
