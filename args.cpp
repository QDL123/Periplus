#include "args.h"

void InitializeArgs::deserialize(std::istream& is) {
    // TODO: break this up and do error checking
    std::cout << "deserializing static data" << std::endl;
    
    this->read_arg<size_t>(&this->d, is);
    this->read_arg<size_t>(&this->max_mem, is);
    this->read_arg<size_t>(&this->nTotal, is);
    this->read_arg<size_t>(&this->size, is);

    // Read delimiter between static and dynamic data
    char temp;
    this->read_arg<char>(&temp, is);
    if (temp != '\n') {
        std::cerr << "Expected delimiter, but didn't find one" << std::endl;
    }

    this->db_url = this->read_dynamic_data<char>(is, this->size);

    // Read the end delimiter
    this->read_arg<char>(&temp, is);
    if (temp != '\r') {
        std::cerr << "Expected end delimiter but didn't find it" << std::endl;
    }
    this->read_arg<char>(&temp, is);
    if (temp != '\n') {
        std::cerr << "Expected end delimiter but didn't find it" << std::endl;
    }
}

Command InitializeArgs::get_command() { return INITIALIZE; }
