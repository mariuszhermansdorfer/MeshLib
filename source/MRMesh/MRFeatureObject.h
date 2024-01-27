#pragma once
#include "MRMeshFwd.h"
#include <variant>

namespace MR
{

using FeaturesPropertyTypesVariant = std::variant<float, Vector3f >;

struct  FeatureObject;

// FeatureObjectSharedProperty struct is designed to represent a shared property of a feature object, enabling the use of generalized getter and setter methods for property manipulation.
// propertyName: A string representing the name of the property.
// getter : A std::function encapsulating a method with no parameters that returns a FeaturesPropertyTypesVariant.This allows for a generic way to retrieve the value of the property.
// setter : A std::function encapsulating a method that takes a FeaturesPropertyTypesVariant as a parameter and returns void.This function sets the value of the property.
// The templated constructor of this struct takes the property name, pointers to the getter and setter member functions, and a pointer to the object( obj ).
// The constructor initializes the propertyName and uses lambdas to adapt the member function pointers into std::function objects that conform to the expected 
// getter and setter signatures.The getter lambda invokes the getter method on the object, and the setter lambda ensures the correct variant type is passed before 
// invoking the setter method.
struct FeatureObjectSharedProperty
{
    std::string propertyName;
    // due to getAllSharedProperties in FeatureObject returns static vector, we need externaly setup object to invoke setter ad getter.
    std::function<FeaturesPropertyTypesVariant( const FeatureObject* objectToInvoke )> getter;
    std::function<void( const FeaturesPropertyTypesVariant&, FeatureObject* objectToInvoke )> setter;

    template <typename T, typename C, typename SetterFunc>
    FeatureObjectSharedProperty(
        std::string name,
        T( C::* m_getter )( ) const,
        SetterFunc m_setter
    ) : propertyName( std::move( name ) ),
        getter
        ( 
            [m_getter] ( const FeatureObject* objectToInvoke ) -> FeaturesPropertyTypesVariant
            {
                return std::invoke( m_getter, dynamic_cast< const C* > ( objectToInvoke ) );
            } 
        )
    {
        if constexpr ( ( std::is_same_v<SetterFunc, void ( C::* )( const T& )> )
            || ( std::is_same_v<SetterFunc, void ( C::* )( T )> ) )
        {
            setter = [m_setter] ( const FeaturesPropertyTypesVariant& v, FeatureObject* objectToInvoke )
            {
                assert( std::holds_alternative<T>( v ) );
                if ( std::holds_alternative<T>( v ) )
                {
                    std::invoke( m_setter, dynamic_cast< C* > ( objectToInvoke ), std::get<T>( v ) );
                }
            };
        }
        else
        {
            static_assert( dependent_false<T>, "Setter function signature unsupported" );
        }
    }
};

/// An interface class which allows feature objects to share setters and getters on their main properties, for convenient presentation in the UI
struct  FeatureObject
{
public:
    FeatureObject() noexcept = default;
    FeatureObject( const FeatureObject& ) noexcept = default;
    FeatureObject( FeatureObject&& ) noexcept = default;
    FeatureObject& operator = ( FeatureObject&& ) noexcept = default;
    FeatureObject& operator = ( const FeatureObject& ) noexcept = default;

    virtual ~FeatureObject() = default;

    /// Create and generate list of bounded getters and setters for the main properties of feature object, together with prop. name for display and edit into UI.
    virtual const std::vector<FeatureObjectSharedProperty>& getAllSharedProperties() const = 0;
};

}