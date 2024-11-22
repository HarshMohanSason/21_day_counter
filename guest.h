#ifndef GUEST_H
#define GUEST_H
 
#include <string> 

class Guest {
public: 
    std::string name; 
    std::string folioNumber; 
    std::string checkInDate; 
    std::string checkOutDate; 
    int totalDaysStayed; 

    Guest(std::string n, std::string folio, std::string checkIn, std::string checkOut, int totalDays)
        : name(n), folioNumber(folio), checkInDate(checkIn), checkOutDate(checkOut), totalDaysStayed(totalDays) {}

    std::string getGuestInfo() {
        return name + " | " + folioNumber + " | " + checkInDate + " | " + checkOutDate + " | " + std::to_string(totalDaysStayed); 
    }
};

#endif

