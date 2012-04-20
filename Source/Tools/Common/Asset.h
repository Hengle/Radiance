// Asset.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <string>
#include <Runtime/Container/HashMap.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

//
// Simple Asset Management System
//
// An asset instance contains the asset object, and it's data, as two seperate class objects.
// For example, for sound, the asset data would be the actual sound samples (from a file for example)
// and the asset instance would be a sound instance which references that data and plays it back.
// This allows multiple assets instances to reference the same source data.
//
// By default the Asset class does not instance the asset and the data seperately, they are considered
// the same, which makes sense for things like textures (which aren't really useful to instance).
//
// The asset data is reference counted and automatically released when all referencing asset
// instances go away.
//

//
// TDerived = The Derived Class
// TType  = The Asset Instance Type
// TData  = The Asset Data Type
//

class IAsset
{
public:
	virtual ~IAsset() {}

	virtual bool Load() = 0;
	virtual bool IsLoaded() const = 0;
};

template <typename TDerived, typename TType, typename TData = TType>
class Asset : public IAsset
{
public:

	typedef TDerived DerivedType;
	typedef TType  AssetType;
	typedef TData  DataType;
	typedef Asset<TDerived, TType, TData> SelfType;

	Asset() : m_asset(0), m_loaded(false)
	{
	}

	Asset(const SelfType &asset) : m_name(asset.m_name), m_loaded(false), m_asset(0)
	{
	}

	explicit Asset(const char *filename) : m_asset(0), m_name(filename), m_loaded(false)
	{
	}

	virtual ~Asset()
	{
		RAD_ASSERT_MSG(!m_asset && !m_refData, "Error: Asset derived type destructor did not call Release()!");
	}

	SelfType &operator = (const SelfType &other)
	{
		Release();
		m_name = other.m_name;
		if (other.m_loaded)
		{
			Load();
		}
		return *static_cast<TDerived*>(this);
	}

	void Bind(const char *filename)
	{
		Release();
		m_name = filename;
	}

	virtual bool Load()
	{
		if (m_name.empty())
		{
			return false;
		}

		if (m_loaded) 
		{
			return m_asset && m_refData;
		}

		m_loaded = true;

		RAD_ASSERT(!m_refData);

		typename DataMap::iterator it = s_dataMap.find(m_name);
		if (it != s_dataMap.end())
		{
			m_refData = (*it).second.lock();
		}

		if (!m_refData)
		{
			DataType *d = LoadData(m_name.c_str());
			if (!d) return false;
			m_refData.reset(new RefData(d));
			m_refData->m_it = s_dataMap.insert(typename DataMap::value_type(m_name, m_refData)).first;
		}

		m_asset = CreateType(m_refData->Data());
		return m_asset != 0;
	}

	const char *Name() const { return m_name.c_str(); }
	virtual bool IsLoaded() const { return m_loaded; }

	void Release()
	{
		if (m_asset && m_refData && ((const void*)m_refData->Data() != (const void*)m_asset))
		{
			DestroyType(m_asset, m_refData->Data());
		}

		m_asset = 0;

		if (m_refData)
		{
			m_refData->SetAsset(this);
			m_refData.reset();
		}
		m_loaded = false;
	}

protected:

	AssetType *GetAsset() const { return m_asset; }
	DataType  *GetData() const { return m_refData->Data(); }
	virtual DataType *LoadData(const char *name) = 0;
	virtual AssetType *CreateType(DataType *data) = 0;
	virtual void DestroyType(AssetType *type, DataType *data) 
	{ 
		if((void*)type != (void*)data) 
		{ 
			delete type; 
		} 
		RAD_NOT_USED(data); 
	}
	virtual void DestroyData(DataType *data) { delete data; }

private:

	class RefData;
	friend class RefData;
	typedef boost::weak_ptr<RefData> RefDataWRef;
	typedef std::string String;
	typedef typename container::hash_map<String, RefDataWRef>::type DataMap;

	typedef boost::shared_ptr<RefData> RefDataRef;

	class RefData
	{
	public:

		RefData(DataType *d) : m_data(d) {}
		~RefData() 
		{ 
			SelfType::s_dataMap.erase(m_it);
			m_ass->DestroyData(m_data); 
		}

		void SetAsset(SelfType *ass) { m_ass = ass; }
		DataType *Data() const { return m_data; }

		typename DataMap::iterator m_it;

	private:

		SelfType *m_ass;
		DataType *m_data;
	};

	String m_name;
	bool m_loaded;
	RefDataRef m_refData;
	AssetType *m_asset;
	static DataMap s_dataMap;
};

template <typename TDerived, typename TType, typename TData>
typename Asset<TDerived, TType, TData>::DataMap Asset<TDerived, TType, TData>::s_dataMap;
