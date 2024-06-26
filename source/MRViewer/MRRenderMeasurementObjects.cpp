#include "MRRenderMeasurementObjects.h"

#include "MRMesh/MRVisualObject.h"

namespace MR
{

MR_REGISTER_RENDER_OBJECT_IMPL( DistanceMeasurementObject, RenderDistanceObject )
RenderDistanceObject::RenderDistanceObject( const VisualObject& object )
    : RenderDimensionObject( object ), object_( &dynamic_cast<const DistanceMeasurementObject&>( object ) )
{}

void RenderDistanceObject::renderUi( const UiRenderParams& params )
{
    Vector3f pointA = object_->getWorldPoint();
    Vector3f pointB = pointA + object_->getWorldDelta();
    task_ = RenderDimensions::LengthTask( params, {}, object_->getFrontColor( object_->isSelected(), params.viewportId ), {
        .points = { pointA, pointB },
        .drawAsNegative = object_->getDrawAsNegative(),
        .showPerCoordDeltas = object_->getPerCoordDeltasMode() != DistanceMeasurementObject::PerCoordDeltas::none,
        .perCoordDeltasAreAbsolute = object_->getPerCoordDeltasMode() == DistanceMeasurementObject::PerCoordDeltas::absolute,
    } );
    params.tasks->push_back( { std::shared_ptr<void>{}, &task_ } ); // A non-owning shared pointer.
}

MR_REGISTER_RENDER_OBJECT_IMPL( RadiusMeasurementObject, RenderRadiusObject )
RenderRadiusObject::RenderRadiusObject( const VisualObject& object )
    : RenderDimensionObject( object ), object_( &dynamic_cast<const RadiusMeasurementObject&>( object ) )
{}

void RenderRadiusObject::renderUi( const UiRenderParams& params )
{
    task_ = RenderDimensions::RadiusTask( params, {}, object_->getFrontColor( object_->isSelected(), params.viewportId ), {
        .center = object_->getWorldCenter(),
        .radiusAsVector = object_->getWorldRadiusAsVector(),
        .normal = object_->getWorldNormal(),
        .drawAsDiameter = object_->getDrawAsDiameter(),
        .isSpherical = object_->getIsSpherical(),
        .visualLengthMultiplier = object_->getVisualLengthMultiplier(),
    } );
    params.tasks->push_back( { std::shared_ptr<void>{}, &task_ } ); // A non-owning shared pointer.
}

MR_REGISTER_RENDER_OBJECT_IMPL( AngleMeasurementObject, RenderAngleObject )
RenderAngleObject::RenderAngleObject( const VisualObject& object )
    : RenderDimensionObject( object ), object_( &dynamic_cast<const AngleMeasurementObject&>( object ) )
{}

void RenderAngleObject::renderUi( const UiRenderParams& params )
{
    task_ = RenderDimensions::AngleTask( params, {}, object_->getFrontColor( object_->isSelected(), params.viewportId ), {
        .center = object_->getWorldPoint(),
        .rays = {
            object_->getWorldRay( false ),
            object_->getWorldRay( true ),
        },
        .isConical = object_->getIsConical(),
        .shouldVisualizeRay = {
            object_->getShouldVisualizeRay( false ),
            object_->getShouldVisualizeRay( true ),
        },
    } );
    params.tasks->push_back( { std::shared_ptr<void>{}, &task_ } ); // A non-owning shared pointer.
}

}
