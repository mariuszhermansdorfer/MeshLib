#include "MRMultiwayICP.h"
#include "MRParallelFor.h"
#include "MRTimer.h"
#include "MRPointToPointAligningTransform.h"
#include "MRPointToPlaneAligningTransform.h"

namespace MR
{

MultiwayICP::MultiwayICP( const Vector<MultiICPObject, MeshOrPointsId>& objects, float samplingVoxelSize ) :
    objs_{ objects }
{
    resamplePoints( samplingVoxelSize );
}

Vector<AffineXf3f, MeshOrPointsId> MultiwayICP::calculateTransformations()
{
    float minDist = std::numeric_limits<float>::max();
    int badIterCount = 0;
    resultType_ = ICPExitType::MaxIterations;

    for ( iter_ = 1; iter_ <= prop_.iterLimit; ++iter_ )
    {
        updatePointPairs_();
        const bool pt2pt = ( prop_.method == ICPMethod::Combined && iter_ < 3 )
            || prop_.method == ICPMethod::PointToPoint;
        if ( !( pt2pt ? p2ptIter_() : p2plIter_() ) )
        {
            resultType_ = ICPExitType::NotFoundSolution;
            break;
        }

        const float curDist = pt2pt ? getMeanSqDistToPoint() : getMeanSqDistToPlane();
        if ( prop_.exitVal > curDist )
        {
            resultType_ = ICPExitType::StopMsdReached;
            break;
        }

        // exit if several(3) iterations didn't decrease minimization parameter
        if ( curDist < minDist )
        {
            minDist = curDist;
            badIterCount = 0;
        }
        else
        {
            if ( badIterCount >= prop_.badIterStopCount )
            {
                resultType_ = ICPExitType::MaxBadIterations;
                break;
            }
            badIterCount++;
        }
    }

    Vector<AffineXf3f, MeshOrPointsId> res;
    res.resize( objs_.size() );
    for ( int i = 0; i < objs_.size(); ++i )
        res[MeshOrPointsId( i )] = objs_[MeshOrPointsId( i )].xf;
    return res;
}

void MultiwayICP::resamplePoints( float samplingVoxelSize )
{
    MR_TIMER;
    pairsPerObj_.clear();
    pairsPerObj_.resize( objs_.size() );

    Vector<VertBitSet, MeshOrPointsId> samplesPerObj( objs_.size() );

    ParallelFor( objs_, [&] ( MeshOrPointsId ind )
    {
        const auto& obj = objs_[ind];
        samplesPerObj[ind] = *obj.meshOrPoints.pointsGridSampling( samplingVoxelSize );
    } );

    for ( MeshOrPointsId i = MeshOrPointsId( 0 ); i < objs_.size(); ++i )
    {
        auto& pairs = pairsPerObj_[i];
        pairs.resize( objs_.size() );
        for ( MeshOrPointsId j = MeshOrPointsId( 0 ); j < objs_.size(); ++j )
        {
            if ( i == j )
                continue;
            auto& thisPairs = pairs[j];
            thisPairs.vec.reserve( samplesPerObj[i].count());
            for ( auto v : samplesPerObj[i] )
                thisPairs.vec.emplace_back().srcVertId = v;
            thisPairs.active.reserve( thisPairs.vec.size() );
            thisPairs.active.clear();
        }
    }
}

float MultiwayICP::getMeanSqDistToPoint() const
{
    MR_TIMER;
    NumSum sum;
    for ( MeshOrPointsId i = MeshOrPointsId( 0 ); i < objs_.size(); ++i )
        for ( MeshOrPointsId j = MeshOrPointsId( 0 ); j < objs_.size(); ++j )
            if ( i != j )
                sum = sum + MR::getSumSqDistToPoint( pairsPerObj_[i][j] );
    return sum.rootMeanSqF();
}

float MultiwayICP::getMeanSqDistToPlane() const
{
    MR_TIMER;
    NumSum sum;
    for ( MeshOrPointsId i = MeshOrPointsId( 0 ); i < objs_.size(); ++i )
        for ( MeshOrPointsId j = MeshOrPointsId( 0 ); j < objs_.size(); ++j )
            if ( i != j )
                sum = sum + MR::getSumSqDistToPlane( pairsPerObj_[i][j] );
    return sum.rootMeanSqF();
}

size_t MultiwayICP::getNumActivePairs() const
{
    size_t num = 0;
    for ( MeshOrPointsId i = MeshOrPointsId( 0 ); i < objs_.size(); ++i )
        for ( MeshOrPointsId j = MeshOrPointsId( 0 ); j < objs_.size(); ++j )
            if ( i != j )
                num = num + MR::getNumActivePairs( pairsPerObj_[i][j] );
    return num;
}

void MultiwayICP::updatePointPairs_()
{
    MR_TIMER;
    for ( MeshOrPointsId i = MeshOrPointsId( 0 ); i < objs_.size(); ++i )
        for ( MeshOrPointsId j = MeshOrPointsId( 0 ); j < objs_.size(); ++j )
            if ( i != j )
                MR::updatePointPairs( pairsPerObj_[i][j], objs_[i].meshOrPoints, objs_[i].xf, objs_[j].meshOrPoints, objs_[j].xf, prop_.cosTreshold, prop_.distThresholdSq, prop_.mutualClosest );
    deactivatefarDistPairs_();
}

void MultiwayICP::deactivatefarDistPairs_()
{
    MR_TIMER;

    for ( int it = 0; it < 3; ++it )
    {
        const auto avgDist = getMeanSqDistToPoint();
        const auto maxDistSq = sqr( prop_.farDistFactor * avgDist );
        if ( maxDistSq >= prop_.distThresholdSq )
            break;
        size_t deactivatedNum = 0;
        for ( MeshOrPointsId i = MeshOrPointsId( 0 ); i < objs_.size(); ++i )
            for ( MeshOrPointsId j = MeshOrPointsId( 0 ); j < objs_.size(); ++j )
                if ( i != j )
                    deactivatedNum += MR::deactivateFarPairs( pairsPerObj_[i][j], maxDistSq );
        if ( deactivatedNum == 0 )
            break; // nothing was deactivated
    }
}

bool MultiwayICP::p2ptIter_()
{
    MR_TIMER;
    Vector<bool, MeshOrPointsId> valid( objs_.size() );
    ParallelFor( objs_, [&] ( MeshOrPointsId id )
    {
        PointToPointAligningTransform p2pt;
        for ( MeshOrPointsId j = MeshOrPointsId( 0 ); j < objs_.size(); ++j )
        {
            if ( j == id )
                continue;
            for ( size_t idx : pairsPerObj_[id][j].active )
            {
                const auto& vp = pairsPerObj_[id][j].vec[idx];
                p2pt.add( vp.srcPoint, vp.tgtPoint, vp.weight );
            }
        }

        AffineXf3f res;
        switch ( prop_.icpMode )
        {
        default:
            assert( false );
            [[fallthrough]];
        case ICPMode::RigidScale:
            res = AffineXf3f( p2pt.findBestRigidScaleXf() );
            break;
        case ICPMode::AnyRigidXf:
            res = AffineXf3f( p2pt.findBestRigidXf() );
            break;
        case ICPMode::OrthogonalAxis:
            res = AffineXf3f( p2pt.findBestRigidXfOrthogonalRotationAxis( Vector3d{ prop_.fixedRotationAxis } ) );
            break;
        case ICPMode::FixedAxis:
            res = AffineXf3f( p2pt.findBestRigidXfFixedRotationAxis( Vector3d{ prop_.fixedRotationAxis } ) );
            break;
        case ICPMode::TranslationOnly:
            res = AffineXf3f( Matrix3f(), Vector3f( p2pt.findBestTranslation() ) );
            break;
        }
        if ( std::isnan( res.b.x ) ) //nan check
        {
            valid[id] = false;
            return;
        }
        objs_[id].xf = res * objs_[id].xf;
        valid[id] = true;
    } );

    return std::all_of( valid.vec_.begin(), valid.vec_.end(), [] ( auto v ) { return v; } );
}

bool MultiwayICP::p2plIter_()
{
    MR_TIMER;
    Vector<bool, MeshOrPointsId> valid( objs_.size() );
    ParallelFor( objs_, [&] ( MeshOrPointsId id )
    {
        Vector3f centroidRef;
        int activeCount = 0;
        for ( MeshOrPointsId j = MeshOrPointsId( 0 ); j < objs_.size(); ++j )
        {
            if ( j == id )
                continue;
            for ( size_t idx : pairsPerObj_[id][j].active )
            {
                const auto& vp = pairsPerObj_[id][j].vec[idx];
                centroidRef += vp.tgtPoint;
                centroidRef += vp.srcPoint;
                ++activeCount;
            }
        }
        if ( activeCount <= 0 )
        {
            valid[id] = false;
            return;
        }
        centroidRef /= float( activeCount * 2 );
        AffineXf3f centroidRefXf = AffineXf3f( Matrix3f(), centroidRef );

        PointToPlaneAligningTransform p2pl;
        for ( MeshOrPointsId j = MeshOrPointsId( 0 ); j < objs_.size(); ++j )
        {
            if ( j == id )
                continue;
            for ( size_t idx : pairsPerObj_[id][j].active )
            {
                const auto& vp = pairsPerObj_[id][j].vec[idx];
                p2pl.add( vp.srcPoint - centroidRef, vp.tgtPoint - centroidRef, vp.tgtNorm, vp.weight );
            }
        }

        AffineXf3f res;
        if ( prop_.icpMode == ICPMode::TranslationOnly )
        {
            res = AffineXf3f( Matrix3f(), Vector3f( p2pl.findBestTranslation() ) );
        }
        else
        {
            PointToPlaneAligningTransform::Amendment am;
            switch ( prop_.icpMode )
            {
            default:
                assert( false );
                [[fallthrough]];
            case ICPMode::RigidScale:
                am = p2pl.calculateAmendmentWithScale();
                break;
            case ICPMode::AnyRigidXf:
                am = p2pl.calculateAmendment();
                break;
            case ICPMode::OrthogonalAxis:
                am = p2pl.calculateOrthogonalAxisAmendment( Vector3d{ prop_.fixedRotationAxis } );
                break;
            case ICPMode::FixedAxis:
                am = p2pl.calculateFixedAxisAmendment( Vector3d{ prop_.fixedRotationAxis } );
                break;
            }

            const auto angle = am.rotAngles.length();
            assert( prop_.p2plAngleLimit > 0 );
            assert( prop_.p2plScaleLimit >= 1 );
            if ( angle > prop_.p2plAngleLimit || am.scale > prop_.p2plScaleLimit || prop_.p2plScaleLimit * am.scale < 1 )
            {
                // limit rotation angle and scale
                am.scale = std::clamp( am.scale, 1 / ( double )prop_.p2plScaleLimit, ( double )prop_.p2plScaleLimit );
                if ( angle > prop_.p2plAngleLimit )
                    am.rotAngles *= prop_.p2plAngleLimit / angle;

                // recompute translation part
                am.shift = p2pl.findBestTranslation( am.rotAngles, am.scale );
            }
            res = AffineXf3f( am.rigidScaleXf() );
        }

        if ( std::isnan( res.b.x ) ) //nan check
        {
            valid[id] = false;
            return;
        }
        objs_[id].xf = centroidRefXf * res * centroidRefXf.inverse() * objs_[id].xf;
        valid[id] = true;
    } );
    return std::all_of( valid.vec_.begin(), valid.vec_.end(), [] ( auto v ) { return v; } );
}

}
