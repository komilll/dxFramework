#pragma once
#ifndef _SAVE_SESSION_H_
#define _SAVE_SESSION_H_

#include "framework.h"

class SaveSession
{
public:
	void UpdateSavedData();
	void TryToLoadTextures();

public:
	std::string m_albedoTextureName;
	std::string m_normalTextureName;
	std::string m_roughnessTextureName;
	std::string m_metallicTextureName;

private:
	const char* AUTOSAVE_MATERIAL_FILENAME{ "material.txt" };
};

#endif // !_SAVE_SESSION_H_
