/*
	Copyright Myles Trevino
	Licensed under the Apache License, Version 2.0
	http://www.apache.org/licenses/LICENSE-2.0
*/


#include <iostream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <codecvt>
#include <Windows.h>


std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;

const std::string input_folder{"Input"};
const std::string output_folder{"Output"};

std::string cwebp_arguments;
std::vector<std::string> paths;
std::vector<std::thread> threads;
std::queue<std::thread::id> finished_thread_ids;
std::mutex completion_mutex;
int completed_count;
int error_count;


// Converts the given string to uppercase.
std::string to_uppercase(std::string string)
{
	std::transform(string.begin(), string.end(), string.begin(), toupper);
	return string;
}


// Converts from UTF-8 to UTF-16.
std::wstring utf8_to_utf16(const std::string& string)
{ return convert.from_bytes(string); }


// Prints the given thread error.
void thread_error(const std::string& path, const std::string& message)
{
	std::lock_guard<std::mutex> completion_mutex_lock{completion_mutex};
	std::cout<<"Error coverting \""<<path<<"\": "<<message<<'\n';
	++error_count;
}


// Converts the given image to WebP.
void thread_function(std::string path)
{
	try
	{
		// Convert.
		std::string output{output_folder+'\\'+path.substr(path.find_first_of('\\')+1)};
		output = output.substr(0, output.find_last_of('.'))+".webp";

		std::filesystem::create_directories(output.substr(0, path.find_last_of('\\')+1));

		const std::wstring parameters{utf8_to_utf16("cwebp "+
			cwebp_arguments+"\""+path+"\" -o \""+output+'"')};

		wchar_t* converted_parameters{new wchar_t[parameters.size()+1]};
		for(int index{}; index < parameters.size(); ++index)
			converted_parameters[index] = parameters[index];
		converted_parameters[parameters.size()] = '\0';

		STARTUPINFO startup_information{sizeof(STARTUPINFO)};
		startup_information.cb = sizeof(startup_information);
		startup_information.dwFlags |= STARTF_USESTDHANDLES;

		PROCESS_INFORMATION process_information;

		if(CreateProcess(L"cwebp.exe", converted_parameters, NULL, NULL, FALSE, NULL,
			NULL, NULL, &startup_information, &process_information))
		{
			WaitForSingleObject(process_information.hProcess, INFINITE);
			CloseHandle(process_information.hProcess);
			CloseHandle(process_information.hThread);
		}

		else throw std::runtime_error{"Failed to launch cwebp. "
			"Make sure cwebp.exe is next to this executable."};

		delete[] converted_parameters;

		// Update the completion status.
		std::lock_guard<std::mutex> completion_mutex_lock{completion_mutex};

		++completed_count;

		std::cout<<"\rCompleted "<<completed_count<<" of "<<paths.size()<<
			" ("<<static_cast<int>(static_cast<float>(completed_count)/
			paths.size()*100+.5f)<<"%).      ";
	}

	// Handle errors.
	catch(const std::exception& exception){ thread_error(path, exception.what()); }
	catch(...){ thread_error(path, "Unhandled exception."); }

	// Add the thread ID to the finished queue.
	std::lock_guard<std::mutex> completion_mutex_lock{completion_mutex};
	finished_thread_ids.push(std::this_thread::get_id());
}


// Main.
int main(int argument_count, char* arguments[])
{
	std::cout<<"WebP Converter 2021.1.27\nCopyright Myles Trevino\nlaventh.com\n\n"
		"Licensed under the Apache License, Version 2.0\n"
		"http://www.apache.org/licenses/LICENSE-2.0\n\n---";

	try
	{
		// Check for arguments.
		if(argument_count == 1) std::cout<<"\n\nNo arguments specified. If you would like to "
			"provide arguments to cwebp, run this executable in the form "
			"<executable name> <cwebp arguments>.";
		
		else
		{
			for(int index{1}; index < argument_count; ++index)
				cwebp_arguments += std::string{arguments[index]}+' ';

			std::cout<<"\n\nArguments: "<<cwebp_arguments;
		}

		// Check that the input folder exists.
		if(!std::filesystem::exists(input_folder))
			throw std::runtime_error{"Create a folder called \""+input_folder+"\" next to "
				"this executable and place the image files you want to convert within there. "
				"For organization, you can place your images in subfolders within the folder "
				"and that structure will be replicated in the \""+output_folder+"\" folder."};

		// Create the output folder.
		if(!std::filesystem::exists(output_folder))
			std::filesystem::create_directories(output_folder);

		// Load the paths.
		for(const std::filesystem::directory_entry& iterator :
			std::filesystem::recursive_directory_iterator(input_folder))
		{
			if(!iterator.is_regular_file() || !iterator.path().has_extension()) continue;
			std::string extension{to_uppercase(iterator.path().extension().u8string())};
			if(extension != ".PNG" && extension != ".JPG" && extension != ".JPEG" &&
				extension != ".TIF" && extension != ".TIFF") continue;

			paths.emplace_back(iterator.path().u8string());
		}

		if(paths.size() < 1) throw std::runtime_error("No supported image files "
			"(PNG, JPG/JPEG, TIF/TIFF) were found within the \""+input_folder+"\" folder.");

		// Launch the threads.
		unsigned thread_count{std::thread::hardware_concurrency()/2};
		std::cout<<"\n\nStarting the conversion using "<<thread_count<<" threads.\n";

		for(const std::string& path : paths)
		{
			// If all threads are currently being used...
			if(threads.size() >= thread_count)
			{
				// Wait for a thread to finish.
				while(finished_thread_ids.size() <= 0)
					std::this_thread::sleep_for(std::chrono::milliseconds(1));

				// Erase the finished thread.
				std::thread::id id{finished_thread_ids.front()};
				finished_thread_ids.pop();

				for(int index{}; index < threads.size(); ++index)
					if(threads[index].get_id() == id)
					{
						threads[index].detach();
						threads.erase(threads.begin()+index);
					}
			}

			// Extract the database on a separate thread.
			threads.emplace_back(thread_function, path);
		}

		// Join the threads that are still running.
		for(std::thread& thread : threads) thread.join();

		std::cout<<"\nFinished with "<<error_count<<" error(s).";
	}

	// Handle errors.
	catch(std::runtime_error& error){ std::cout<<"\n\nFatal Error: "<<error.what(); }
	catch(...){ std::cout<<"\n\nFatal unhandled exeption."; }

	// Display the exit prompt.
	std::cout<<"\n\n---\n\n";
	system("pause");
}
