set(SOURCES
	LocalBoundary.hpp
	Mesh.cpp
	Mesh.hpp
	MeshNodes.cpp
	MeshNodes.hpp
	VTUWriter.cpp
	VTUWriter.hpp
)

prepend_current_path(SOURCES)
polyfem_copy_headers(${SOURCES})
polyfem_set_source_group(${SOURCES})
target_sources(polyfem PRIVATE ${SOURCES})
