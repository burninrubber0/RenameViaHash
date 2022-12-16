// Renames all files in a folder based on Bundle resource IDs
// Usage: RenameViaHash names.txt input_folder
// Where names.txt is a list of file names and resource names separated by
// newlines and formatted as FILE_NAME|RESOURCE_NAME. For example:
// AI.DAT|WorldMapData
// TRK_UNIT0_GR.BNDL|TRK_UNIT0_Passby

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <binaryio/binaryreader.hpp>

#include "CRC.h"

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cout << "RenameViaHash by burninrubber0\n\n"
			<< "Renames all files in a folder based on Bundle resource IDs.\n"
			<< "Usage:\n  RenameViaHash names.txt input_folder\n\n"
			<< "  names.txt is a list of file names and resource names\n"
			<< "  separated by newlines and formatted "
			<< "as FILE_NAME|RESOURCE_NAME.\n"
			<< "  For example:\n"
			<< "  AI.DAT|WorldMapData\n"
			<< "  TRK_UNIT0_GR.BNDL|TRK_UNIT0_list\n"
			<< "  VEHICLES/VEH_PUSMC01_AT.BIN|PUSMC01_AttribSys\n\n";

		return 0;
	}

	// Set paths from args
	std::filesystem::path namesPath = argv[1];
	std::filesystem::path folderPath = argv[2];
	if (!std::filesystem::exists(folderPath))
	{
		std::cerr << "Error: Specified folder does not exist.";
		return 1;
	}

	// Read names
	std::ifstream namesFile;
	namesFile.open(namesPath, std::ios::in);
	if (!namesFile)
	{
		std::cerr << "Error: Failed to open names file.";
		namesFile.close();
		return 1;
	}
	std::vector<std::string> lines;
	std::string str;
	while(std::getline(namesFile, str, '\n'))
	{
		if (str.size() > 0)
			lines.push_back(str);
	}

	// Separate lines into file and resource names
	std::vector<std::string> fileNames;
	std::vector<std::string> resourceNames;
	for (int i = 0; i < lines.size(); ++i)
	{
		size_t pos = lines.at(i).find('|');
		fileNames.push_back(lines[i].substr(0, pos));
		resourceNames.push_back(
			lines[i].substr(pos + 1, lines[i].size() - pos + 1));
	}

	// Generate IDs from resource names
	std::vector<uint64_t> resourceIds;
	for (int i = 0; i < resourceNames.size(); ++i)
	{
		std::string strName = resourceNames[i];
		std::transform(
			strName.begin(), strName.end(), strName.begin(), tolower);
		uint64_t crc =
			CRC::Calculate(strName.c_str(), strName.size(), CRC::CRC_32());
		resourceIds.push_back(crc);
	}

	// Read each bundle
	for (const auto& entry
		: std::filesystem::directory_iterator(folderPath))
	{
		if (entry.is_regular_file())
		{
			// Create buffer and reader
			std::ifstream inFile;
			inFile.open(entry.path(), std::ios::in | std::ios::binary);
			const uint64_t fileSize = std::filesystem::file_size(entry.path());
			inFile.seekg(0, std::ios::beg);
			const auto& buffer =
				std::make_shared<std::vector<uint8_t>>(fileSize);
			inFile.read(reinterpret_cast<char*>(buffer->data()), fileSize);
			inFile.close();
			auto reader = binaryio::BinaryReader(buffer);
			reader.SetBigEndian(true); // Remove if doing PC

			// Read in bundle info
			reader.Seek(0x10);
			uint32_t numResources = reader.Read<uint32_t>();
			uint32_t entriesPtr = reader.Read<uint32_t>();
			std::vector<uint64_t> bundleResourceIds;
			for (int i = 0; i < numResources; ++i)
			{
				reader.Seek(entriesPtr + i * 0x40);
				bundleResourceIds.push_back(reader.Read<uint64_t>());
			}

			// If a resource ID is recognized, mark it to be renamed
			bool willRename = false;
			std::string newFileName;
			for (int i = 0; i < bundleResourceIds.size(); ++i)
			{
				for (int j = 0; j < resourceIds.size(); ++j)
				{
					if (bundleResourceIds[i] == resourceIds[j])
					{
						willRename = true;
						newFileName = fileNames[j];
					}
				}
			}

			// Rename files
			if (willRename == true)
			{
				std::string dir = entry.path().parent_path().string();
				std::string fileName = entry.path().stem().string();
				auto extIter = std::find(
					newFileName.begin(), newFileName.end(), '.');
				size_t extPos = std::distance(newFileName.begin(), extIter);
				newFileName.insert(extPos, '.' + fileName);
				newFileName.insert(0, "renamed\\");

				// Create folders if they don't exist
				for (int i = 0; i < newFileName.size(); ++i)
				{
					if (newFileName[i] == '/')
						newFileName[i] = '\\';
				}
				int numDirs = 0;
				size_t folderPos = 0;
				std::string folderDir;
				for (int i = 0; i < newFileName.size(); ++i)
				{
					if (newFileName[i] == '\\')
						numDirs++;
				}
				if (numDirs != 0)
				{
					for (int i = 0; i < numDirs; ++i)
					{
						folderPos = newFileName.find('\\', folderPos + 1);
						folderDir = newFileName.substr(0, folderPos);
						if (!std::filesystem::exists(
							dir + '\\' + folderDir))
							std::filesystem::create_directory(
								dir + '\\' + folderDir);
					}
				}

				std::filesystem::path newPath =
					dir + '\\' + newFileName;
				std::filesystem::rename(entry.path(), newPath);

				std::cout << "Renamed " << entry.path().filename() << " to "
					<< newPath.filename() << '\n';
			}
		}
	}

	std::cout << "\nDone.";

	return 0;
}