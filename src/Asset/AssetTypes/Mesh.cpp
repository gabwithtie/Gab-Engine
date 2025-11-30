#include "Mesh.h"

gbe::asset::Mesh::Mesh(std::filesystem::path path) : BaseAsset(path) {

}

gbe::asset::Mesh* gbe::asset::Mesh::ImportMesh(std::filesystem::path outpathpath)
{
	
	std::filesystem::path destpathpath("cache/models/");
	std::filesystem::path destmetapath("cache/models/");

	std::string filename = outpathpath.filename().string();
	std::string metafilename = filename + ".gbe";

	destpathpath /= filename;
	destmetapath /= metafilename;

	asset::FileUtil::Copy(outpathpath, destpathpath);

	auto importdata = asset::data::MeshImportData{
					.path = filename
	};

	asset::serialization::gbeParser::ExportClass(importdata, destmetapath);

	auto newmesh = new asset::Mesh(destmetapath);

	return newmesh;
}