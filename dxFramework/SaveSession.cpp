#include "SaveSession.h"
#include <fstream>
#include <iostream>
#include <string>

void SaveSession::UpdateSavedData()
{
	if (std::ofstream output{ AUTOSAVE_MATERIAL_FILENAME })
	{
		output << "ALBEDO\n" << m_albedoTextureName << "\n";
		output << "NORMAL\n" << m_normalTextureName << "\n";
		output << "ROUGHNESS\n" << m_roughnessTextureName << "\n";
		output << "METALLIC\n" << m_metallicTextureName << std::endl;
	}
}

void SaveSession::TryToLoadTextures()
{
	std::string line;
	if (std::ifstream input{ AUTOSAVE_MATERIAL_FILENAME })
	{
		while (getline(input, line))
		{
			if (line == "ALBEDO") {
				if (getline(input, line)) m_albedoTextureName = line;
			}
			else if (line == "NORMAL") {
				if (getline(input, line)) m_normalTextureName = line;
			}
			else if (line == "ROUGHNESS") {
				if (getline(input, line)) m_roughnessTextureName = line;
			}
			else if (line == "METALLIC") {
				if (getline(input, line)) m_metallicTextureName = line;
			}
		}
	}
}
