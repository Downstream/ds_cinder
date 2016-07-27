#include "model_maker.h"

#include <map>
#include <sstream>
#include <fstream>
#include <iostream>

#include <Poco/File.h>
#include <Poco/Path.h>

#include <ds/math/random.h>
#include <ds/app/environment.h>
#include <ds/debug/logger.h>
#include <ds/query/query_client.h>
#include <ds/util/string_util.h>


namespace ds {

namespace {

const std::string baseHeader = \
"#pragma once\n#ifndef DS_MODEL_AUTOGENERATED_TableName\n"\
"#define DS_MODEL_AUTOGENERATED_TableName" \
"\n\n#include <ds/data/resource.h>\n#include <memory>\n#include <vector>\n#include <cinder/Vector.h>\n\n" \
"CUSTOM_INCLUDES" \
"\n\n" \
"namespace ds {\n" \
"namespace model{\n" \
"\n\n" \
"FORWARD_DECLARES" \
"\n\n" \
"/**\n" \
"* \\class ds::model::TableNameRef\n" \
"*			Auto-generated data model for TableName\n" \
"*			Don't manually edit this file. Instead drop the source yaml file onto the yaml_importer in the utility folder of ds_cinder. \n"
"*/\n" \
"class TableNameRef {\n" \
"public:\n" \
"\n" \
"	TableNameRef();\n" \
"\n"
"COLUMN_GETTERS\n" \
"\n" \
"COLUMN_SETTERS\n" \
"\n" \
"\n" \
"private:\n" \
"	class Data;\n" \
"	std::shared_ptr<Data>	mData;\n" \
"};\n" \
"\n" \
"} // namespace model\n" \
"} // namespace ds\n" \
"\n" \
"#endif\n";

const std::string baseCpp = \
"#include \"TableName_model.h\" \n" \
"\n" \
"CUSTOM_INCLUDES" \
"\n" \
"namespace ds {\n" \
"namespace model {\n" \
"namespace {\n" \
"const int							EMPTY_INT = 0;\n" \
"const unsigned int					EMPTY_UINT = 0;\n" \
"const float							EMPTY_FLOAT = 0.0f;\n" \
"const std::wstring					EMPTY_WSTRING;\n" \
"const ds::Resource					EMPTY_RESOURCE;\n" \
"CUSTOM_EMPTY_DATA\n" \
"}\n" \
"\n" \
"/**\n" \
"* \\class ds::model::Data\n" \
"*/\n" \
"class TableNameRef::Data {\n" \
"public:\n" \
"	Data()\n"\
"INITIALIZE_DATA_MEMBERS"\
"{}\n" \
"\n" \
"DATA_MEMBERS\n" \
"\n" \
"};\n" \
"\n" \
"TableNameRef::TableNameRef(){}\n" \
"\n" \
"\n" \
"IMP_GETTERS\n" \
"\n" \
"IMP_SETTERS\n" \
"\n" \
"\n" \
"} // namespace model\n" \
"} // namespace ds";

const std::string headerGetter = "	const DATA_TYPE& getColumnName() const;";
const std::string headerSetter = "	TableNameRef& setColumnName(const DATA_TYPE& ColumnName);";
const std::string dataMember = "DATA_TYPE mColumnName;";
const std::string impGetter = "const DATA_TYPE& TableNameRef::getColumnName() const {\n" \
"	if(!mData) return EMPTY_EMPTYDATATYPE; \n" \
"	return mData->mColumnName; \n" \
"}";
const std::string impSetter = "TableNameRef& TableNameRef::setColumnName(const DATA_TYPE& ColumnName){\n" \
"	if(!mData) mData.reset(new Data()); \n" \
"	if(mData) mData->mColumnName = ColumnName; \n" \
"	return *this; \n" \
"}";

const std::string intDataType = "int";
const std::string uintDataType = "unsigned int";
const std::string floatDataType = "float";
const std::string stringDataType = "std::wstring";
const std::string resourceDataType = "ds::Resource";
}

/**
* \class ds::ModelMaker
*/
ModelMaker::ModelMaker() {
	mYamlFileLocation = "";
}

std::string ModelMaker::replaceString(std::string& fullString, std::string toReplace, std::string replaceWith){
	return(fullString.replace(fullString.find(toReplace), toReplace.length(), replaceWith));
}

std::string ModelMaker::replaceAllString(std::string& fullString, std::string toReplace, std::string replaceWith){
	if(toReplace == replaceWith){
		DS_LOG_WARNING("you can't replace the name of a thing with the thing. Change your column name from " << replaceWith << " to something else");
		return "ERROR replacing string. Check the yaml importer.";
	}
	std::string returnString = fullString;
	while(returnString.find(toReplace) != std::string::npos)	{
		returnString = replaceString(returnString, toReplace, replaceWith);
	}

	return returnString;
}

void ModelMaker::run() {
	if(mYamlFileLocation.length() < 1){
		DS_LOG_WARNING("No file specified in model maker");
		return;
	}

	std::cout << "Parsing yaml file: " << mYamlFileLocation << std::endl;

	mYamlLoadService.mFileLocation = mYamlFileLocation;
	mYamlLoadService.run();

	Poco::Path parentDir = Poco::Path(mYamlFileLocation).makeParent();
	

	std::vector<ModelModel> models = mYamlLoadService.mOutput;


	if(models.empty()){
		DS_LOG_WARNING("No valid models loaded in model maker.");
		mOutputStream << "No valid models loaded in model maker. Check the yml files you supplied." << std::endl;
		return;
	}

	for(auto it = models.begin(); it < models.end(); ++it){
		ModelModel& mm = (*it);

		// make the first letter of the table name uppercase
		std::string tableName = mm.getTableName();

		if(tableName.empty()){
			DS_LOG_WARNING("Model maker got a blank table!");
			mOutputStream << "Model maker found a blank table" << std::endl;
			continue;
		}
		std::transform(tableName.begin(), tableName.begin() + 1, tableName.begin(), ::toupper);

		// push all the nearby getters/setters/members into stringstreams
		// headers
		std::stringstream sCustomIncludes;
		std::stringstream sCustomForwardDeclares;
		std::stringstream sHeaderGetters;
		std::stringstream sHeaderSetters;

		// imps
		std::stringstream sCustomImpIncludes;
		std::stringstream sDataMembers;
		std::stringstream sCustomEmptyData;
		std::stringstream sImpGetters;
		std::stringstream sImpSetters;

		// strings for each column
		std::string thisHeaderGetter;
		std::string thisHeaderSetter;
		std::string thisEmptyData;
		std::string thisDataMember;
		std::string thisImpGetter;
		std::string thisImpSetter;

		std::map<std::string, bool> emptyDataClasses;
		std::map<std::string, std::string> dataMemberInitializers;

		// add custom includes specified in the YAML
		if(!mm.getCustomInclude().empty()){
			sCustomIncludes << "#include " << mm.getCustomInclude() << std::endl;
		}

		// make all 'resource' flagged columns into proper resources
		for(auto mit = mm.getResourceColumns().begin(); mit < mm.getResourceColumns().end(); ++mit){
			for(auto cit = mm.getColumns().begin(); cit < mm.getColumns().end(); ++cit){

				// if this column is the resource one, set it's type to resource and move on
				if((*cit).getColumnName() == (*mit) && ((*cit).getType() == ModelColumn::Integer || (*cit).getType() == ModelColumn::UnsignedInt)) {
					ModelColumn* mc = const_cast<ModelColumn*>(&(*cit));
					if(mc){
						mc->setType(ModelColumn::Resource);
					}
					break;
				}
			}
		}

		// add special columns for all relations
		for(auto rit = mm.getRelations().begin(); rit < mm.getRelations().end(); ++rit){
			const ModelRelation& mr = (*rit);
			if(mr.getType() == ModelRelation::Invalid) continue;

			// create the class name out of the foreign table name, e.g. StoryRef
			std::string foreignTable = mr.getForeignKeyTable();
			std::transform(foreignTable.begin(), foreignTable.begin() + 1, foreignTable.begin(), ::toupper);
			std::stringstream foreignClass;
			foreignClass << foreignTable << "Ref";

			// create the empty data name from the class name. e.g. EMPTY_STORY or EMPTY_STORY_ELEMENT_VECTOR
			std::string foreignEmptyType = foreignClass.str();
			std::transform(foreignEmptyType.begin(), foreignEmptyType.end(), foreignEmptyType.begin(), ::toupper);
			std::stringstream foreignEmptyTypeFull;
			foreignEmptyTypeFull << foreignEmptyType;

			std::stringstream foreignDataType;

			if(mr.getType() == ModelRelation::Many){
				foreignEmptyTypeFull << "_VECTOR";
				foreignDataType << "std::vector<" << foreignClass.str() << ">";

				// the 'many' types need to have a header include
				sCustomIncludes << "#include \"" << getFileName(mr.getForeignKeyTable(), true) << "\"" << std::endl;
			} else {
				foreignDataType << foreignClass.str();

				// single types need to be forward declared to avoid circular refs
				sCustomForwardDeclares << "class " << foreignClass.str() << ";" << std::endl;
				sCustomImpIncludes << "#include \"" << getFileName(mr.getForeignKeyTable(), true) << "\"" << std::endl;
			}
			
			ModelColumn mc;
			if(mr.getLocalKeyColumn().empty()){
				mc.setColumnName(foreignClass.str());
			} else {
				mc.setColumnName(mr.getLocalKeyColumn());
			}
			mc.setType(ModelColumn::Custom);
			mc.setCustomDataType(foreignDataType.str());
			mc.setCustomEmptyDataName(foreignEmptyTypeFull.str());
			mm.addColumn(mc);

		}


		// add the getters, setters, empty data and data members for each column to the stringstreams
		for(auto cit = mm.getColumns().begin(); cit < mm.getColumns().end(); ++cit){

			thisHeaderGetter = headerGetter;
			thisHeaderSetter = headerSetter;
			thisEmptyData = "";
			thisDataMember = dataMember;
			thisImpGetter = impGetter;
			thisImpSetter = impSetter;

			std::string dataType = "";

			const ModelColumn& mc = (*cit);

			// invalid columns get tossed
			if(mc.getType() == ModelColumn::Invalid){
				continue;

				// if it's an int, check for unsigned int too
			} else if(mc.getType() == ModelColumn::Integer || mc.getType() == ModelColumn::UnsignedInt){
				if(mc.getIsUnsigned()){
					dataType = uintDataType;
					thisEmptyData = "UINT";

				} else {
					dataType = intDataType;
					thisEmptyData = "INT";
				}

			} else if(mc.getType() == ModelColumn::Float){
				dataType = floatDataType;
				thisEmptyData = "FLOAT";

			} else if(mc.getType() == ModelColumn::String){
				dataType = stringDataType;
				thisEmptyData = "WSTRING";

			} else if(mc.getType() == ModelColumn::Resource){
				dataType = resourceDataType;
				thisEmptyData = "RESOURCE";

			} else if(mc.getType() == ModelColumn::Custom){
				dataType = mc.getCustomDataType();
				thisEmptyData = mc.getCustomEmptyDataName();
				if(emptyDataClasses.find(thisEmptyData) == emptyDataClasses.end()){
					emptyDataClasses[thisEmptyData] = true;
					sCustomEmptyData << "const " << mc.getCustomDataType() << " EMPTY_" << mc.getCustomEmptyDataName() << ";" << std::endl;
				}
			}


			std::string columnName = mc.getColumnName();
			std::transform(columnName.begin(), columnName.begin() + 1, columnName.begin(), ::toupper);

			dataMemberInitializers[columnName] = thisEmptyData;

			thisImpGetter = replaceAllString(thisImpGetter, "EMPTYDATATYPE", thisEmptyData);

			thisHeaderGetter = replaceAllString(thisHeaderGetter, "DATA_TYPE", dataType);
			thisHeaderSetter = replaceAllString(thisHeaderSetter, "DATA_TYPE", dataType); 	
			thisDataMember = replaceAllString(thisDataMember, "DATA_TYPE", dataType);
			thisImpGetter = replaceAllString(thisImpGetter, "DATA_TYPE", dataType);
			thisImpSetter = replaceAllString(thisImpSetter, "DATA_TYPE", dataType);

			sHeaderGetters << replaceAllString(thisHeaderGetter, "ColumnName", columnName) << std::endl;
			sHeaderSetters << replaceAllString(thisHeaderSetter, "ColumnName", columnName) << std::endl;
			sDataMembers << replaceAllString(thisDataMember, "ColumnName", columnName) << std::endl;
			sImpGetters << replaceAllString(thisImpGetter, "ColumnName", columnName) << std::endl;
			sImpSetters << replaceAllString(thisImpSetter, "ColumnName", columnName) << std::endl;

		}

		// add initializers to empty data when Data() is constructed
		std::stringstream memberInitializers;
		bool firstInitializer = true;
		for(auto it = dataMemberInitializers.begin(); it != dataMemberInitializers.end(); ++it){
			if(firstInitializer){
				memberInitializers << "\t: m";
				firstInitializer = false;
			} else {
				memberInitializers << "\t, m";
			}

			memberInitializers << it->first << "(EMPTY_" << it->second << ")\n";
		}

		// add all the stringstreams to the header and put in the table name / model name
		std::string header = baseHeader;
		header = replaceAllString(header, "FORWARD_DECLARES", sCustomForwardDeclares.str());
		header = replaceAllString(header, "CUSTOM_INCLUDES", sCustomIncludes.str());
		header = replaceAllString(header, "COLUMN_GETTERS", sHeaderGetters.str());
		header = replaceAllString(header, "COLUMN_SETTERS", sHeaderSetters.str());
		header = replaceAllString(header, "TableName", tableName);

		//std::cout << std::endl << std::endl << header << std::endl << std::endl;


		std::string imp = baseCpp;
		imp = replaceAllString(imp, "CUSTOM_INCLUDES", sCustomImpIncludes.str());
		imp = replaceAllString(imp, "CUSTOM_EMPTY_DATA", sCustomEmptyData.str());
		imp = replaceAllString(imp, "INITIALIZE_DATA_MEMBERS", memberInitializers.str());
		imp = replaceAllString(imp, "DATA_MEMBERS", sDataMembers.str());
		imp = replaceAllString(imp, "IMP_GETTERS", sImpGetters.str());
		imp = replaceAllString(imp, "IMP_SETTERS", sImpSetters.str());
		imp = replaceAllString(imp, "TableName", tableName);

		//	std::cout << std::endl << std::endl << imp << std::endl << std::endl;


		// write out the header and implementation
		std::stringstream filename;
		filename << parentDir.toString() << getFileName(mm.getTableName(), true);

		mOutputStream << "Wrote header to: " << filename.str() << std::endl;

		std::ofstream headerWriter;
		headerWriter.open(ds::Environment::expand(filename.str()));
		headerWriter << header;
		headerWriter.close();

		filename.str("");

		filename << parentDir.toString() << getFileName(mm.getTableName(), false);
		mOutputStream << "Wrote implementation to: " << filename.str() << std::endl;

		std::ofstream impWriter;
		impWriter.open(ds::Environment::expand(filename.str()));
		impWriter << imp;
		impWriter.close();
	}

	mOutputText = mOutputStream.str();
}

std::string ModelMaker::getFileName(const std::string& tableName, const bool header){
	std::string lower_case_table = tableName;
	std::transform(lower_case_table.begin(), lower_case_table.end(), lower_case_table.begin(), ::tolower);
	std::stringstream filename;
	filename << lower_case_table << "_model.";
	if(header){
		filename << "h";
	} else {
		filename << "cpp";
	}
	return filename.str();
}


} // namespace ds
