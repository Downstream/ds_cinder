#pragma once
#ifndef DS_CONTENT_CONTENT_MODEL
#define DS_CONTENT_CONTENT_MODEL

#include <cinder/Color.h>
#include <cinder/Rect.h>
#include <cinder/Vector.h>
#include <ds/data/resource.h>
#include <map>
#include <memory>
#include <vector>


namespace ds {

namespace ui {
class SpriteEngine;
}

namespace model {


/**
 * \class ContentProperty
 * \brief A single property on a ContentModel.
 *		 For instance, this could be ContentProperty("longitude", 123.456); or ContentProperty("title", "The title of this thing")
 */
class ContentProperty {
  public:
	ContentProperty();
	ContentProperty(const std::string& name, const std::string& value);
	ContentProperty(const std::string& name, const std::string& value, const int& valueInt, const double& valueDouble);

	/// Get the name of this property
	const std::string& getName() const;
	void			   setName(const std::string& name);

	const std::string& getValue() const;
	void			   setValue(const std::string& value);
	void			   setValue(const std::wstring& value);
	void			   setValue(const int& value);
	void			   setValue(const double& value);
	void			   setValue(const float& value);
	void			   setValue(const ci::Color& value);
	void			   setValue(const ci::ColorA& value);
	void			   setValue(const ci::vec2& value);
	void			   setValue(const ci::vec3& value);
	void			   setValue(const ci::Rectf& value);

	ds::Resource getResource() const;
	void		 setResource(const ds::Resource& resource);

	/// The name, value and resource are equal
	bool operator==(const ContentProperty&) const;

	/// ------- This value, type converted when called --------- //

	bool   getBool() const;
	int	getInt() const;
	float  getFloat() const;
	double getDouble() const;

	/// The Engine is supplied to look up named colors
	const ci::Color  getColor(ds::ui::SpriteEngine&) const;
	const ci::ColorA getColorA(ds::ui::SpriteEngine&) const;

	const std::string& getString() const;  // same as getValue(), but supplied here for convenience
	const std::wstring getWString() const;

	const ci::vec2  getVec2() const;
	const ci::vec3  getVec3() const;
	const ci::Rectf getRect() const;

  protected:
	std::string				  mName;
	std::string				  mValue;
	int						  mIntValue;
	double					  mDoubleValue;
	std::shared_ptr<Resource> mResource;
};


/**
 * \class ContentModelRef
 * \brief A nodal hierarchy-based generic content model
 *		 Each ContentModelRef points to an underlying Data object by a shared pointer, so you can copy these instances around
 *freely Each ContentModelRef has:
 *			* A series of properties (e.g. Title, Body, ImageResource, Latitude, etc)
 *			* A series of children, all ContentModelRefs themselves, to build a hierarchy
 *			* A name, which can be used to look up models. Typically the name of a table from a database
 *			* A label, for identifying models to humans
 *			* An Id, which has no guarantee of uniqueness, that typically comes from a database, and can be used to look up models
 */
class ContentModelRef {
  public:
	/// TODO: auto validation (e.g. exists, is a date, media meets certain qualifications, etc)
	/// TODO: remove child

	ContentModelRef();
	ContentModelRef(const std::string& name, const int id = 0, const std::string& label = "");

	/// Enables doing `if (mModel) ...` to check if model is valid
	operator bool() const { return !empty(); }

	/// Get the id for this item
	const int& getId() const;
	void	   setId(const int& id);

	/// Get the name of this item
	/// Name is generally inherited by the table or thing this belongs to
	/// This is used in the getChildByName()
	const std::string& getName() const;
	void			   setName(const std::string& name);

	/// Get the label for this item
	/// This is a helpful name or display name for this thing
	const std::string& getLabel() const;
	void			   setLabel(const std::string& label);

	/// Get the user data pointer.
	void* getUserData() const;
	void  setUserData(void* userData);

	/// If this item has no data, value, name, id, properties or children
	const bool empty() const;
	
	/// Removes all data (children, properties, name, id, etc). After calling this, empty() will return true
	void clear();

	/// Makes a copy of this content.
	/// This make a brand new ContentModelRef with a different underlying data object, so you can modify the two independently
	ds::model::ContentModelRef duplicate() const;

	/// Tests if this ContentModelRef has the same Id, Name, Label and underlying data pointer
	bool operator==(const ContentModelRef&) const;

	bool operator!=(const ContentModelRef&) const;

	/// Use this for looking stuff up only. Recommend using the other functions to manage the list
	const std::map<std::string, ContentProperty>& getProperties();
	void										  setProperties(const std::map<std::string, ContentProperty>& newProperties);

	/// This can return an empty property, which is why it's const.
	/// If you want to modify a property, use the setProperty() function
	const ContentProperty getProperty(const std::string& propertyName);
	const std::string	  getPropertyValue(const std::string& propertyName);
	bool				  getPropertyBool(const std::string& propertyName);
	int					  getPropertyInt(const std::string& propertyName);
	float				  getPropertyFloat(const std::string& propertyName);
	double				  getPropertyDouble(const std::string& propertyName);
	/// The Engine is supplied to look up named colors
	const ci::Color		  getPropertyColor(ds::ui::SpriteEngine&, const std::string& propertyName);
	const ci::ColorA      getPropertyColorA(ds::ui::SpriteEngine&, const std::string& propertyName);
	const std::string     getPropertyString(const std::string& propertyName);
	const std::wstring    getPropertyWString(const std::string& propertyName);
	const ci::vec2		  getPropertyVec2(const std::string& propertyName);
	const ci::vec3		  getPropertyVec3(const std::string& propertyName);
	const ci::Rectf		  getPropertyRect(const std::string& propertyName);
	ds::Resource	      getPropertyResource(const std::string& propertyName);

	/// Set the property with a given name
	void setProperty(const std::string& propertyName, ContentProperty& theProp);
	void setProperty(const std::string& propertyName, char* value); // for constants as properties, eg mModel.setProperty("sample", "value"); in c++
	void setProperty(const std::string& propertyName, const std::string& value);
	void setProperty(const std::string& propertyName, const std::wstring& value);
	void setProperty(const std::string& propertyName, const int& value);
	void setProperty(const std::string& propertyName, const double& value);
	void setProperty(const std::string& propertyName, const float& value);
	void setProperty(const std::string& propertyName, const ci::Color& value);
	void setProperty(const std::string& propertyName, const ci::ColorA& value);
	void setProperty(const std::string& propertyName, const ci::vec2& value);
	void setProperty(const std::string& propertyName, const ci::vec3& value);
	void setProperty(const std::string& propertyName, const ci::Rectf& value);
	void setPropertyResource(const std::string& propertyName, const ds::Resource& resource);

	/// property lists are stored separately from regular properties
	const std::map<std::string, std::vector<ContentProperty>>& getAllPropertyLists();
	const std::vector<ContentProperty>&	getPropertyList(const std::string& propertyName);
	std::vector<bool>				getPropertyListBool(const std::string& propertyName);
	std::vector<int>				getPropertyListInt(const std::string& propertyName);
	std::vector<float>				getPropertyListFloat(const std::string& propertyName);
	std::vector<double>				getPropertyListDouble(const std::string& propertyName);
	std::vector<ci::Color>			getPropertyListColor(ds::ui::SpriteEngine&, const std::string& propertyName);
	std::vector<ci::ColorA>			getPropertyListColorA(ds::ui::SpriteEngine&, const std::string& propertyName);
	std::vector<std::string>		getPropertyListString(const std::string& propertyName);
	std::vector<std::wstring>		getPropertyListWString(const std::string& propertyName);
	std::vector<ci::vec2>			getPropertyListVec2(const std::string& propertyName);
	std::vector<ci::vec3>			getPropertyListVec3(const std::string& propertyName);
	std::vector<ci::Rectf>			getPropertyListRect(const std::string& propertyName);

	/// Returns the list as a delimiter-separated string
	std::string						getPropertyListAsString(const std::string& propertyName, const std::string& delimiter = "; ");

	/// Adds this to a property list
	void addPropertyToList(const std::string& propertyListName, const std::string& value);

	/// Replaces a property list 
	void setPropertyList(const std::string& propertyListName, const std::vector<std::string>& value);
	void setPropertyList(const std::string& propertyListName, const std::vector<ContentProperty>& value);

	/// Clears the list for a specific property
	void clearPropertyList(const std::string& propertyName);

	/// Gets all of the children
	/// Don't modify the children here, use the other functions
	const std::vector<ContentModelRef>& getChildren() const;

	/// If no children exist, returns an empty data model
	/// If index is greater than the size of the children, returns the last child
	ContentModelRef getChild(const size_t index);

	/// Get the first child that matches this id
	/// If no children exist or match that id, returns an empty data model
	ContentModelRef getChildById(const int id);

	/// Get the first child that matches this name
	/// Can get nested children using dot notation. for example: getChildByName("the_stories.chapter_one.first_paragraph");
	/// If no children exist or match that id, returns an empty data model
	ContentModelRef getChildByName(const std::string& childName) const;

	/// Looks through the entire tree to find a child that matches the name and id.
	/// For instance, if you have a branched tree several levels deep and need to find a specific node.
	/// Depends on children having a consistent name and unique id.
	ContentModelRef getDescendant(const std::string& childName, const int childId);

	/// Looks through all direct children, and returns all children that have a given label.
	/// Useful for models that have children from more than one table
	/// \note By default, labels are in the form "sql_table_name row"
	std::vector<ContentModelRef> getChildrenWithLabel(const std::string& label);

	/// Get first direct decendant where 'propertyName' has a value of 'propertyValue'
	/// Returns an empty model if no match is found
	ContentModelRef findChildByPropertyValue(const std::string& propertyName, const std::string& propertyValue);

	/// Adds this child to the end of this children list, or at the index supplied
	void addChild(ContentModelRef datamodel);
	void addChild(ContentModelRef datamodel, const size_t index);

	/// If there's a direct descendant with the name, replaces it, adds it if it doesn't exist
	void replaceChild(ds::model::ContentModelRef datamodel);

	/// Is there a child with this name?
	bool hasChild(const std::string& name) const;
	bool hasDirectChild(const std::string& name) const;

	/// Is there at least one child?
	bool hasChildren() const;

	/// Replaces all children
	void setChildren(std::vector<ds::model::ContentModelRef> children);

	/// Removes all children
	void clearChildren();

	/// Adds a reference map with the corresponding string name
	void setReferences(const std::string& referenceName, std::map<int, ds::model::ContentModelRef>& reference);

	/// Gets a map of all the references for the given name. If you need to modify the map, make a copy and set it again using setReference
	const std::map<int, ds::model::ContentModelRef>& getReferences(const std::string& referenceName);

	/// Returns a content model from a specific reference by the reference name and the node id
	ds::model::ContentModelRef getReference(const std::string& referenceName, const int nodeId);

	/// Clears the reference map at the specified name
	void clearReferences(const std::string& name);

	/// Removes all references
	void clearAllReferences();

	/// Logs this, it's properties, and all it's children recursively
	void printTree(const bool verbose, const std::string& indent = "");

  private:
	void createData();
	class Data;
	std::shared_ptr<Data> mData;
};

}  // namespace model
}  // namespace ds

#endif

#pragma once
#pragma once
