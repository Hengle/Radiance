// EditorContentTexture.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../EditorUtils.h"
#include "EditorContentTexture.h"
#include "EditorContentProperties.h"
#include "../EditorLineEditDialog.h"
#include "../../../Assets/TextureParser.h"
#include <Runtime/ImageCodec/Dds.h>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>

namespace tools {
namespace editor {
namespace content_property_details {

namespace {

const char *FormatToString(UReg format)
{
#define F(_x) \
	case image_codec::Format_##_x: \
		return #_x

#define DDS(_x) \
	case image_codec::dds::Format_##_x: \
		return #_x

	switch (format)
	{
		F(A8);
		F(PAL8_RGB555);
		F(PAL8_RGB565);
		F(PAL8_RGB888);
		F(PAL8_RGBA8888);
		F(RGB555);
		F(BGR555);
		F(RGB565);
		F(BGR565);
		F(RGB888);
		F(BGR888);
		F(RGBA4444);
		F(BGRA4444);
		F(RGBA5551);
		F(BGRA5551);
		F(RGBA8888);
		F(BGRA8888);
		DDS(DXT1);
		DDS(DXT3);
		DDS(DXT5);
		DDS(FP16);
		DDS(FP32);
		DDS(FP416);
		DDS(FP432);
		DDS(S8888);
	}

	return "Unknown";
}

} // namespace

RADENG_API void RADENG_CALL AddTextureProperties(
	PropertyList &l,
	const pkg::Package::Entry::Ref &e, 
	int flags, 
	QWidget &widget
)
{
	if (flags != 0)
		return; // no platform specific properties

	pkg::Asset::Ref asset = e->Asset(pkg::Z_ContentBrowser);
	if (!asset)
		return;

	int r = asset->Process(
		xtime::TimeSlice::Infinite,
		pkg::P_Info|pkg::P_Unformatted|pkg::P_NoDefaultMedia
	);

	if (r == pkg::SR_Success)
	{ // extract width/height/bpp
		asset::TextureParser::Ref parser(asset::TextureParser::Cast(asset));
		if (!parser || !parser->headerValid)
			return;

		asset::TextureParser::Header header = *parser->header.get();

		l.append(
			new (ZEditor) ReadOnlyTraits<int>::PropertyType(
				header.width,
				Property::UserContext::Ref(),
				"Source.Width",
				widget
			)
		);

		l.append(
			new (ZEditor) ReadOnlyTraits<int>::PropertyType(
				header.height,
				Property::UserContext::Ref(),
				"Source.Height",
				widget
			)
		);

		l.append(
			new (ZEditor) ReadOnlyTraits<int>::PropertyType(
				header.numMips,
				Property::UserContext::Ref(),
				"Source.NumMips",
				widget
			)
		);

		l.append(
			new (ZEditor) ReadOnlyTraits<QString>::PropertyType(
				FormatToString(header.format),
				Property::UserContext::Ref(),
				"Source.Format",
				widget
			)
		);
	}
}

RADENG_API pkg::IdVec RADENG_CALL CreateTextures(QWidget *parent, const pkg::Package::Ref &pkg, pkg::IdVec &sel)
{
	pkg::IdVec ids;
	QFileDialog fd(parent, "Import Textures");

	QString nativePrefix;
	
	{
		nativePrefix = QString("9:/") + QString::fromWCharArray(Files()->hddRoot);
		wchar_t native[file::MaxFilePathLen+1];
		if (!file::ExpandToNativePath(nativePrefix.toStdWString().c_str(), native, file::MaxFilePathLen+1))
			return ids;
		nativePrefix = QString::fromWCharArray(native);
		for (int i = 0; i < nativePrefix.length(); ++i)
		{
			if (nativePrefix[i] == '\\')
				nativePrefix[i] = '/';
		}
	}

	fd.setDirectory(nativePrefix + QString("/Textures"));
	fd.setNameFilter("All Files (*.*)");
	fd.setFileMode(QFileDialog::ExistingFiles);

	// lookup file filter from asset definition.
	pkg::KeyDef::MapRef defs = Packages()->KeyDefsForType(asset::AT_Texture);
	pkg::KeyDef::Map::const_iterator it = defs->find(String("Source.File"));
	if (it != defs->end())
	{
		pkg::KeyDef::Pair::Map::const_iterator it2 = it->second->pairs.find(String("filter"));
		if (it2 != it->second->pairs.end())
		{
			const String *s = static_cast<const String*>(it2->second.val);
			if (s)
				fd.setNameFilter(s->c_str());
		}
	}
	
	bool validated = false;
	QStringList files;

	while (fd.exec())
	{
		files = fd.selectedFiles();

		validated = true;

		foreach(QString s, files)
		{
			validated = s.startsWith(nativePrefix, Qt::CaseInsensitive);
			if (!validated)
				break;
		}

		if (validated)
			break;

		validated = false;

		QMessageBox::critical(
			parent,
			"Invalid path",
			"Selected files must be located inside game directory."
		);
	}

	if (!validated)
		return ids;

	QMessageBox::StandardButton b = QMessageBox::question(
		parent,
		"Question",
		"Would you like to create materials for the texture(s) you are importing?",
		QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
		QMessageBox::Yes
	);

	if (b == QMessageBox::Cancel)
		return ids;

	bool createMaterials = b == QMessageBox::Yes;
	bool autoRename = false;
	bool hasConflicts = false;

	{
		QString conflictsMessage;

		foreach(QString path, files)
		{
			QFileInfo fileInfo(path);
			QString name = fileInfo.baseName();
			QString matName = name + "_M";

			if (pkg->FindEntry(name.toAscii().constData()))
			{
				if (!conflictsMessage.isEmpty())
					conflictsMessage += QString(RAD_NEWLINE)+name;
				else
					conflictsMessage = name;
			}
			
			if(createMaterials && pkg->FindEntry(matName.toAscii().constData()))
			{
				if (!conflictsMessage.isEmpty())
					conflictsMessage += QString(RAD_NEWLINE)+matName;
				else
					conflictsMessage = matName;
			}
			
		}

		if (!conflictsMessage.isEmpty())
		{
			QString m = QString("One or more textures or materials selected for import conflict with assets in this package. Would you like to automatically rename the conflicting items during import?"RAD_NEWLINE""RAD_NEWLINE"The following items have conflicts:"RAD_NEWLINE"%1").arg(conflictsMessage);
			b = QMessageBox::warning(
				parent,
				"Conflicts",
				m,
				QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
			);

			if (b == QMessageBox::Cancel)
				return ids; // user wants to abort due to conflicts

			hasConflicts = true;
			autoRename = b == QMessageBox::Yes;
		}
	}

	// Create each texture and associated material.

	foreach(QString path, files)
	{
		QFileInfo fileInfo(path);
		QString name = fileInfo.baseName();
		
		pkg::Package::Entry::Ref texture;

		for (int i = 2;; ++i)
		{
			texture = pkg->CreateEntry(
				name.toAscii().constData(),
				asset::AT_Texture
			);

			if (texture)
			{
				ids.push_back(texture->id);
				if (!createMaterials)
					sel.push_back(texture->id);
				break;
			}

			if (!autoRename)
				break;

			name = QString("%1_%2").arg(fileInfo.baseName()).arg(i);
		}

		if (!texture)
			continue;

		// Set Source.File input
		pkg::KeyDef::Ref def = texture->FindKeyDef(String(), String("Source.File"));
		if (def)
		{
			pkg::KeyVal::Ref key = def->CreateKey(0);
			texture->AddKey(key, true);

			String *s = static_cast<String*>(key->val);
			if (s)
				*s = path.right(path.length()-nativePrefix.length()-1).toAscii().constData();
		}

		// Parse header, see if we need to disable mipmap wrap?
		{
			pkg::Asset::Ref t = texture->Asset(pkg::Z_Unique);
			if (t)
			{
				int r = t->Process(
					xtime::TimeSlice::Infinite,
					pkg::P_Info|pkg::P_Unformatted|pkg::P_NoDefaultMedia
				);

				if (r == pkg::SR_Success)
				{
					asset::TextureParser::Ref parser = asset::TextureParser::Cast(t);

					bool pow2 = IsPowerOf2(parser->header->width) && IsPowerOf2(parser->header->height);
					bool square = parser->header->width == parser->header->height;

					if (!pow2)
					{
						def = texture->FindKeyDef(String(), String("Wrap.S"));
						if (def)
						{
							pkg::KeyVal::Ref key = def->CreateKey(0);
							texture->AddKey(key, true);
							bool *b = static_cast<bool*>(key->val);
							if (b)
								*b = false;
						}

						def = texture->FindKeyDef(String(), String("Wrap.T"));
						if (def)
						{
							pkg::KeyVal::Ref key = def->CreateKey(0);
							texture->AddKey(key, true);
							bool *b = static_cast<bool*>(key->val);
							if (b)
								*b = false;
						}

						def = texture->FindKeyDef(String(), String("Wrap.R"));
						if (def)
						{
							pkg::KeyVal::Ref key = def->CreateKey(0);
							texture->AddKey(key, true);
							bool *b = static_cast<bool*>(key->val);
							if (b)
								*b = false;
						}

						def = texture->FindKeyDef(String(), String("Mipmap"));
						if (def)
						{
							pkg::KeyVal::Ref key = def->CreateKey(0);
							texture->AddKey(key, true);
							bool *b = static_cast<bool*>(key->val);
							if (b)
								*b = false;
						}
					}

					if (pow2 && square && 
						((parser->header->format == image_codec::Format_RGB888) ||
						 (parser->header->format == image_codec::Format_RGBA8888)))// enable compression
					{
						def = texture->FindKeyDef(String(), String("Compression"));
						if (def)
						{ // not a K_Global property
							for (int t = pkg::P_FirstTarget; t <= pkg::P_LastTarget; t <<= 1)
							{
								pkg::KeyVal::Ref key = def->CreateKey(t);
								texture->AddKey(key, true);
								String *s = static_cast<String*>(key->val);
								if (s)
									*s = "DXT1/PVR2";
							}
						}
					}
				}
			}
		}

		texture->UpdateModifiedTime();

		if (createMaterials)
		{
			name = fileInfo.baseName();
			QString matName = name + "_M";

			pkg::Package::Entry::Ref mat;

			for (int i = 2;; ++i)
			{
				mat = pkg->CreateEntry(
					matName.toAscii().constData(),
					asset::AT_Material
				);

				if (mat)
				{
					ids.push_back(mat->id);
					sel.push_back(mat->id);
					break;
				}

				if (!autoRename)
					break;

				matName = QString("%1_%2_M").arg(name).arg(i);
			}

			if (!mat)
				continue;

			// Set Texture1.Source.Texture input...
			pkg::KeyDef::Ref def = mat->FindKeyDef(String(), String("Texture1.Source.Texture"));
			if (def)
			{
				pkg::KeyVal::Ref key = def->CreateKey(0);
				mat->AddKey(key, true);

				String *s = static_cast<String *>(key->val);
				if (s)
				{
					*s = texture->path.get();
					if (def->style&pkg::K_Import)
						mat->MapImport(key);
				}
			}

			mat->UpdateModifiedTime();
		}
	}

	if (hasConflicts && !autoRename && !ids.empty())
		QMessageBox::information(parent, "Warning", "Some selected textures or materials were not imported due to conflicts.");

	return ids;
}

} // content_property_details
} // editor
} // tools
