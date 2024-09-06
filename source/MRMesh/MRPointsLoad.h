#pragma once

#include "MRMeshFwd.h"
#include "MRExpected.h"
#include "MRIOFilters.h"
#include "MRPointsLoadSettings.h"

#include <filesystem>

namespace MR::PointsLoad
{

/// \defgroup PointsLoadGroup Points Load
/// \addtogroup IOGroup
/// \{

/// loads from .csv, .asc, .xyz, .txt file
MRMESH_API Expected<PointCloud> fromText( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromText( std::istream& in, const PointsLoadSettings& settings = {} );

/// loads from Laser scan plain data format (.pts) file
MRMESH_API Expected<PointCloud> fromPts( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromPts( std::istream& in, const PointsLoadSettings& settings = {} );

#ifndef MRMESH_NO_OPENCTM
/// loads from .ctm file
MRMESH_API Expected<PointCloud> fromCtm( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromCtm( std::istream& in, const PointsLoadSettings& settings = {} );
#endif

/// loads from .ply file
MRMESH_API Expected<PointCloud> fromPly( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromPly( std::istream& in, const PointsLoadSettings& settings = {} );

/// loads from .obj file
MRMESH_API Expected<PointCloud> fromObj( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromObj( std::istream& in, const PointsLoadSettings& settings = {} );

#if !defined( __EMSCRIPTEN__ ) && !defined( MRMESH_NO_E57 )
/// loads from .e57 file
MRMESH_API Expected<PointCloud> fromE57( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromE57( std::istream& in, const PointsLoadSettings& settings = {} );
#endif

#if !defined( MRMESH_NO_LAS )
/// loads from .las file
MRMESH_API Expected<PointCloud> fromLas( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromLas( std::istream& in, const PointsLoadSettings& settings = {} );
#endif

MRMESH_API Expected<PointCloud> fromDxf( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromDxf( std::istream& in, const PointsLoadSettings& settings = {} );

/// detects the format from file extension and loads points from it
MRMESH_API Expected<PointCloud> fromAnySupportedFormat( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
/// extension in `*.ext` format
MRMESH_API Expected<PointCloud> fromAnySupportedFormat( std::istream& in, const std::string& extension, const PointsLoadSettings& settings = {} );

/// \}

} // namespace MR::PointsLoad
