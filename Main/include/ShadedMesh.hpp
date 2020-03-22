#pragma once
#include "stdafx.h"

class ShadedMesh {
public:
	ShadedMesh(const String& name);
	ShadedMesh(const String& name, const String& path);
	ShadedMesh();

	void Draw();
	void SetData(Vector<MeshGenerators::SimpleVertex>& data);
	void AddTexture(const String& name, const String& file);
	void AddSkinTexture(const String& name, const String& file);
	void SetBlendMode(const MaterialBlendMode& mode);
	void SetPrimitiveType(const PrimitiveType& type);
	void SetOpaque(bool opaque);

	template<typename T>
	void SetParam(const String& name, const T& value) {
		m_params.SetParameter(name, value);
	}

	static int lNew(struct lua_State* L);

	static Map<String, void*> FunctionMap;

private:
	Mesh m_mesh;
	Material m_material;
	MaterialParameterSet m_params;
	Map<String, Texture> m_textures;
};