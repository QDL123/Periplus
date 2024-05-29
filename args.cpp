#include "args.h"

void InitializeArgs::deserialize(std::istream& is) {
    // TODO: break this up and do error checking
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

    this->read_end_delimiter(is);


    std::cout << "INITIALIZE: " << this->d << ", " << this->max_mem << ", " << this->nTotal << ", ";
    for (size_t i = 0; i < this->size; i++) {
        std::cout << this->db_url[i];
    }
    std::cout << std::endl;
}

void InitializeArgs::deserialize_static(std::istream& is) {
    std::cout << "ERROR: INITIALIZE DESERIALIZE_STATIC WAS CALLED" << std::endl;
}

void InitializeArgs::deserialize_dynamic(std::istream& is) {
    std::cout << "ERROR: INITIALIZE DESERIALIZE_DYNAMIC WAS CALLED" << std::endl;
}

size_t InitializeArgs::get_static_size() { return static_size; }

Command InitializeArgs::get_command() { return INITIALIZE; }


void TrainArgs::deserialize(std::istream& is) {
    this->read_arg<size_t>(&this->size, is);

    // This works assuming this->size is the number of bytes which is sizeof(float) * d * n
    size_t nFloats = this->size / sizeof(float);
    char temp;
    this->read_arg<char>(&temp, is);
    if (temp != '\n') {
        std::cerr << "Expected delimiter, but didn't find one" << std::endl;
    }

    this->training_data = this->read_dynamic_data<float>(is, nFloats);
    for (int i = 0; i < nFloats; i++) {
        if (this->training_data[i] == 0) {
            std::cout << "first 0 at " << i << std::endl;
            break;
        }
    }
    this->read_end_delimiter(is);

    std::cout << "TRAIN:  " << nFloats << std::endl;
    // for (size_t i = 0; i < nFloats; i++) {
    //     std::cout << this->training_data[i] << ", ";
    // }
    std::cout << std::endl;
}

void TrainArgs::deserialize_static(std::istream& is) {
    std::cout << "Deserializing static data" << std::endl;
    this->read_arg<size_t>(&this->size, is);
    // std::cout << "Size of size_t: " << sizeof(this->size) << std::endl;
    char temp;
    this->read_arg<char>(&temp, is);
    if (temp != '\n') {
        std::cerr << "Expected delimiter, but didn't find one" << std::endl;
    } else {
        std::cout << "Consumed static delimiter" << std::endl;
    }
}

void TrainArgs::deserialize_dynamic(std::istream& is) {
    std::cout << "Deserializing dynamic data" << std::endl;
    size_t nFloats = this->size / sizeof(float);
    this->training_data = this->read_dynamic_data<float>(is, nFloats);
    std::cout << "Last vector: " << this->training_data[nFloats - 2] << ", " << this->training_data[nFloats - 1] << std::endl;
    this->read_end_delimiter(is);
}

size_t TrainArgs::get_static_size() { return static_size; }

Command TrainArgs::get_command() { return TRAIN; }