// CMSC 341 - Spring 2024 - Project 4
#include "vacdb.h"
#include <math.h>
#include <random>
#include <vector>
#include <algorithm>
#include <ctime>     //used to get the current time
// We can use the Random class to generate the test data randomly!
enum RANDOM {UNIFORMINT, UNIFORMREAL, NORMAL, SHUFFLE};
class Random {
public:
    Random(int min, int max, RANDOM type=UNIFORMINT, int mean=50, int stdev=20) : m_min(min), m_max(max), m_type(type)
    {
        if (type == NORMAL){
            //the case of NORMAL to generate integer numbers with normal distribution
            m_generator = std::mt19937(m_device());
            //the data set will have the mean of 50 (default) and standard deviation of 20 (default)
            //the mean and standard deviation can change by passing new values to constructor 
            m_normdist = std::normal_distribution<>(mean,stdev);
        }
        else if (type == UNIFORMINT) {
            //the case of UNIFORMINT to generate integer numbers
            // Using a fixed seed value generates always the same sequence
            // of pseudorandom numbers, e.g. reproducing scientific experiments
            // here it helps us with testing since the same sequence repeats
            m_generator = std::mt19937(10);// 10 is the fixed seed value
            m_unidist = std::uniform_int_distribution<>(min,max);
        }
        else if (type == UNIFORMREAL) { //the case of UNIFORMREAL to generate real numbers
            m_generator = std::mt19937(10);// 10 is the fixed seed value
            m_uniReal = std::uniform_real_distribution<double>((double)min,(double)max);
        }
        else { //the case of SHUFFLE to generate every number only once
            m_generator = std::mt19937(m_device());
        }
    }
    void setSeed(int seedNum){
        // we have set a default value for seed in constructor
        // we can change the seed by calling this function after constructor call
        // this gives us more randomness
        m_generator = std::mt19937(seedNum);
    }

    void getShuffle(vector<int> & array){
        // the user program creates the vector param and passes here
        // here we populate the vector using m_min and m_max
        for (int i = m_min; i<=m_max; i++){
            array.push_back(i);
        }
        shuffle(array.begin(),array.end(),m_generator);
    }

    void getShuffle(int array[]){
        // the param array must be of the size (m_max-m_min+1)
        // the user program creates the array and pass it here
        vector<int> temp;
        for (int i = m_min; i<=m_max; i++){
            temp.push_back(i);
        }
        std::shuffle(temp.begin(), temp.end(), m_generator);
        vector<int>::iterator it;
        int i = 0;
        for (it=temp.begin(); it != temp.end(); it++){
            array[i] = *it;
            i++;
        }
    }

    int getRandNum(){
        // this function returns integer numbers
        // the object must have been initialized to generate integers
        int result = 0;
        if(m_type == NORMAL){
            //returns a random number in a set with normal distribution
            //we limit random numbers by the min and max values
            result = m_min - 1;
            while(result < m_min || result > m_max)
                result = m_normdist(m_generator);
        }
        else if (m_type == UNIFORMINT){
            //this will generate a random number between min and max values
            result = m_unidist(m_generator);
        }
        return result;
    }

    double getRealRandNum(){
        // this function returns real numbers
        // the object must have been initialized to generate real numbers
        double result = m_uniReal(m_generator);
        // a trick to return numbers only with two deciaml points
        // for example if result is 15.0378, function returns 15.03
        // to round up we can use ceil function instead of floor
        result = std::floor(result*100.0)/100.0;
        return result;
    }

    string getRandString(int size){
        // the parameter size specifies the length of string we ask for
        // to use ASCII char the number range in constructor must be set to 97 - 122
        // and the Random type must be UNIFORMINT (it is default in constructor)
        string output = "";
        for (int i=0;i<size;i++){
            output = output + (char)getRandNum();
        }
        return output;
    }
    
    private:
    int m_min;
    int m_max;
    RANDOM m_type;
    std::random_device m_device;
    std::mt19937 m_generator;
    std::normal_distribution<> m_normdist;//normal distribution
    std::uniform_int_distribution<> m_unidist;//integer uniform distribution
    std::uniform_real_distribution<double> m_uniReal;//real uniform distribution

};


class Tester {
public:
    static void testInsertion();
    static void testFindError(VacDB& db);
    static void testFindNonColliding();
    static void testFindColliding();
    static void testRemoveNonColliding();
    static void testRemoveColliding();
    static void testRehashTriggerOnInsertion();
    static void testRehashCompletionOnInsertion();
    static void testRehashTriggerOnDeletion();
    static void testRehashCompletionOnDeletion();
    static void testHighVolumeInsertion();
    static void testConsistencyPostRehash();
    static void testInsertionDuplicates();
    static void testInsertionBoundarySerialNumbers();
    static void testRemoveNonExistent();
    static void testRemoveAcrossTables();
};



void Tester::testInsertion() {
    cout << "Testing Insertion Operation..." << endl;

    // Assuming the table starts empty and using a fixed hash function
    VacDB db(101, [](std::string key) -> unsigned int {
        return static_cast<unsigned int>(hash<string>{}(key) % 101);
    }, LINEAR);

    vector<string> keys = {"Alice", "Bob", "Carol", "Dave", "Eve"};
    bool allInsertedCorrectly = true;
    int expectedNewSize = keys.size();  // Since we know how many we're inserting

    for (const auto& key : keys) {
        Patient newPatient(key, 1000 + hash<string>{}(key) % 1000);
        bool inserted = db.insert(newPatient);
        if (!inserted) {
            allInsertedCorrectly = false;
            cout << "Failed to insert: " << key << endl;
        }
    }

    int actualNewSize = db.getCurrentSize();  // Get the size after all insertions
    if (actualNewSize != expectedNewSize) {
        std::cout << "Size did not update correctly. Expected size: " << expectedNewSize << ", Actual size: " << actualNewSize << std::endl;
        allInsertedCorrectly = false;
    }

    cout << "Insertion Test: " << (allInsertedCorrectly ? "PASS" : "FAIL") << endl;
}







void Tester::testFindError(VacDB& db) {
    cout << "Testing Find Operation for Error Case..." << endl;
    // Test for a non-existing patient
    const Patient result = db.getPatient("NonExistent", 999);
    bool pass = result.getSerial() == 0; // Assuming serial number 0 signifies not found
    cout << "Find Error Test: " << (pass ? "PASS" : "FAIL") << endl;
}



void Tester::testFindNonColliding() {
        cout << "Testing Find Operation for Non-Colliding Keys..." << endl;

        // Initialize the database with a suitable hash function
        VacDB db(101, [](string key) -> unsigned int {
            return static_cast<unsigned int>(hash<string>{}(key) % 101);
        }, LINEAR);

        // List of keys to test
        vector<pair<string, int>> testData = {
            {"Alice", 1001},
            {"Bob", 1002},
            {"Carol", 1003},
            {"Dave", 1004},
            {"Eve", 1005}
        };

        // Insert non-colliding keys
        for (const auto& [key, serial] : testData) {
            Patient newPatient(key, serial);
            db.insert(newPatient);  // Assuming insert works correctly
        }

        // Verify each key can be found and matches the expected details
        bool allFoundCorrectly = true;
        for (const auto& [key, serial] : testData) {
            const Patient foundPatient = db.getPatient(key, serial);
            if (!(foundPatient.getSerial() == serial && foundPatient.getKey() == key)) {
                allFoundCorrectly = false;
                cout << "Failed to find or incorrect details for key: " << key << endl;
            }
        }

        cout << "Find Non-Colliding Keys Test: " << (allFoundCorrectly ? "PASS" : "FAIL") << endl;
    }

void Tester::testFindColliding() {
    cout << "Testing Find Operation for Colliding Keys..." << endl;
    
    // Correct the lambda to match the expected signature by casting to `unsigned int`
    hash_fn myHash = [](string key) -> unsigned int { 
        return static_cast<unsigned int>(hash<string>{}(key) % 5); 
    }; // Force collisions by hashing to a very small range

    VacDB db(101, myHash, LINEAR);

    bool foundAll = true;
    for (int i = 0; i < 10; i++) {
        string key = "Patient" + to_string(i);
        Patient newPatient(key, 2000 + i);
        db.insert(newPatient);
        const Patient foundPatient = db.getPatient(key, 2000 + i);
        foundAll &= (foundPatient.getSerial() == 2000 + i);
    }
    cout << "Find Colliding Test: " << (foundAll ? "PASS" : "FAIL") << endl;
}


void Tester::testRemoveNonColliding() {
        cout << "Testing Remove Operation for Non-Colliding Keys..." << endl;

        // Initialize the database with a suitable hash function
        VacDB db(101, [](string key) -> unsigned int {
            return static_cast<unsigned int>(hash<string>{}(key) % 101);
        }, LINEAR);

        // List of keys to test
        vector<pair<string, int>> testData = {
            {"Alice", 1001},
            {"Bob", 1002},
            {"Carol", 1003},
            {"Dave", 1004},
            {"Eve", 1005}
        };

        // Insert non-colliding keys
        for (const auto& [key, serial] : testData) {
            Patient newPatient(key, serial);
            db.insert(newPatient);  // Assuming insert works correctly
        }

        // Now remove each key and verify removal
        bool allRemovedCorrectly = true;
        int initialSize = db.getCurrentSize();
        for (const auto& [key, serial] : testData) {
            Patient patientToRemove(key, serial);
            if (!db.remove(patientToRemove)) {
                allRemovedCorrectly = false;
                cout << "Failed to remove: " << key << endl;
            } else {
                // Confirm the patient is no longer in the database
                Patient foundPatient = db.getPatient(key, serial);
                if (foundPatient.getUsed()) {  // Assumes getUsed() tells if the patient is still considered present
                    allRemovedCorrectly = false;
                    cout << "Patient not properly removed from database: " << key << endl;
                }
            }
        }

        int newSize = db.getCurrentSize();
        if (newSize != initialSize - testData.size()) {
            cout << "Size did not update correctly after removal. Expected size: " << (initialSize - testData.size()) << ", Actual size: " << newSize << endl;
            allRemovedCorrectly = false;
        }

        cout << "Remove Non-Colliding Keys Test: " << (allRemovedCorrectly ? "PASS" : "FAIL") << endl;
    }


void Tester::testRemoveColliding() {
    cout << "Testing Remove Operation for Colliding Keys..." << endl;
    
    // First, insert items that will collide
    VacDB db(101, [](string key) -> unsigned int { return std::hash<string>{}(key) % 10; }, LINEAR); // Ensuring collisions
    bool insertedAll = true;
    for (int i = 0; i < 10; i++) {
        string key = "Patient" + to_string(i);
        Patient newPatient(key, 2000 + i, true);
        insertedAll &= db.insert(newPatient);
    }
    if (!insertedAll) {
        cout << "Failed to insert all items for removal test." << endl;
        return;  // Exit if not all items were inserted correctly
    }

    // Now, attempt to remove them
    bool removedAll = true;
    for (int i = 0; i < 10; i++) {
        string key = "Patient" + to_string(i);
        Patient patientToRemove(key, 2000 + i);
        if (!db.remove(patientToRemove)) {
            cout << "Failed to remove: " << key << endl;
            removedAll = false;
        } else {
            const Patient foundPatient = db.getPatient(key, 2000 + i);
            if (foundPatient.getUsed()) {  // Check if the patient is still marked as used
                cout << "Patient still exists after removal: " << key << endl;
                removedAll = false;
            }
        }
    }
    cout << "Remove Colliding Test: " << (removedAll ? "PASS" : "FAIL") << endl;
}


void Tester::testRemoveNonExistent() {
    cout << "Testing Remove Operation for Non-Existent Keys..." << endl;
    VacDB db(101, [](string key) -> unsigned int {
        return static_cast<unsigned int>(hash<string>{}(key) % 101);
    }, LINEAR);

    // Try to remove a non-existent patient
    Patient nonExistent("NonExistent", 9999);
    bool removed = db.remove(nonExistent);
    if (!removed) {
        cout << "Correctly identified non-existent patient: PASS" << endl;
    } else {
        cout << "Failed to identify non-existent patient: FAIL" << endl;
    }
}



void Tester::testRehashTriggerOnInsertion() {
    cout << "Testing Rehash Trigger on Insertion..." << endl;

    // Assuming the initial capacity and the fact that rehashing should occur at a certain load factor
    VacDB db(7, [](string key) -> unsigned int { 
        return static_cast<unsigned int>(hash<string>{}(key)); 
    }, LINEAR);  // Small capacity for easier rehash trigger

    int countToInsert = ceil(7 * 0.75) + 1;  // More than 75% capacity to trigger rehash

    // Insert patients and track their details for verification post-rehash
    vector<pair<string, int>> patientDetails;
    for (int i = 0; i < countToInsert; i++) {
        string name = "Patient" + to_string(i);
        int serial = 3000 + i;
        db.insert(Patient(name, serial));
        patientDetails.push_back({name, serial});
    }

    // Additional inserts to ensure rehash is triggered
    db.insert(Patient("Extra1", 9999));
    db.insert(Patient("Extra2", 9998));

    // Verify all patients are still correctly accessible and have the correct data
    bool rehashLikelyTriggered = true;
    for (const auto& [name, serial] : patientDetails) {
        Patient retrieved = db.getPatient(name, serial);
        if (retrieved.getSerial() != serial || retrieved.getKey() != name || !retrieved.getUsed()) {
            rehashLikelyTriggered = false;
            break;
        }
    }

    cout << "Rehash Trigger on Insertion Test: " << (rehashLikelyTriggered ? "PASS" : "FAIL") << endl;
}


void Tester::testRehashCompletionOnInsertion() {
    cout << "Testing Rehash Completion on Insertion..." << endl;
    VacDB db(7, [](string key) -> unsigned int { 
        return static_cast<unsigned int>(hash<string>{}(key)); 
    }, LINEAR);  // Small table to trigger rehash easily

    int countToInsert = ceil(7 * 0.75) + 1;  // Insert more than 75% capacity to likely trigger rehash

    vector<pair<string, int>> insertedPatients;
    for (int i = 0; i < countToInsert; i++) {
        string key = "Patient" + to_string(i);
        int serial = 3000 + i;
        db.insert(Patient(key, serial));
        insertedPatients.push_back({key, serial});
    }

    // Force a rehash by inserting more items beyond capacity
    db.insert(Patient("Extra1", 9999));
    db.insert(Patient("Extra2", 9998));

    // Verify all initially inserted patients are still correctly accessible
    bool allDataCorrect = true;
    for (const auto& p : insertedPatients) {
        const Patient foundPatient = db.getPatient(p.first, p.second);
        allDataCorrect &= (foundPatient.getSerial() == p.second);
    }

    cout << "Rehash Completion on Insertion Test: " << (allDataCorrect ? "PASS" : "FAIL") << endl;
}



void Tester::testRehashTriggerOnDeletion() {
    cout << "Testing Rehash Trigger on Deletion..." << endl;

    // We need a known starting point, let's fill the database with a predictable pattern
    VacDB db(11, [](string key) -> unsigned int { // small prime number capacity
        return static_cast<unsigned int>(hash<string>{}(key));
    }, LINEAR);

    int initialEntries = 10;
    for (int i = 0; i < initialEntries; i++) {
        db.insert(Patient("Patient" + to_string(i), 3000 + i));
    }

    // Now, delete enough patients to potentially trigger rehashing
    int countToRemove = ceil(initialEntries * 0.8);  // Removing 80% of initial entries
    for (int i = 0; i < countToRemove; i++) {
        db.remove(Patient("Patient" + to_string(i), 3000 + i));
    }

    // If rehashing occurred, there should be less fragmentation. Check if remaining data is accessible
    // This checks if entries at the start are not reachable, indicating potential rehash
    const Patient remainingPatient = db.getPatient("Patient" + to_string(countToRemove), 3000 + countToRemove);
    bool rehashLikelyTriggered = (remainingPatient.getSerial() == 3000 + countToRemove);

    cout << "Rehash Trigger on Deletion Test: " << (rehashLikelyTriggered ? "PASS" : "FAIL") << endl;
}


void Tester::testRehashCompletionOnDeletion() {
    cout << "Testing Rehash Completion on Deletion..." << endl;

    // Refill the database to a known state
    VacDB db(11, [](string key) -> unsigned int {
        return static_cast<unsigned int>(hash<string>{}(key));
    }, LINEAR);

    int initialEntries = 10;
    for (int i = 0; i < initialEntries; i++) {
        db.insert(Patient("Patient" + to_string(i), 3000 + i));
    }

    // Delete entries to trigger a rehash
    int countToRemove = ceil(initialEntries * 0.8);
    for (int i = 0; i < countToRemove; i++) {
        db.remove(Patient("Patient" + to_string(i), 3000 + i));
    }

    // Check the integrity of the remaining data
    bool allDataCorrect = true;
    for (int i = countToRemove; i < initialEntries; i++) {
        const Patient foundPatient = db.getPatient("Patient" + to_string(i), 3000 + i);
        if (foundPatient.getSerial() != 3000 + i) {
            allDataCorrect = false;
            break;
        }
    }

    cout << "Rehash Completion on Deletion Test: " << (allDataCorrect ? "PASS" : "FAIL") << endl;
}

void Tester::testHighVolumeInsertion() {
    cout << "Testing High Volume Insertion..." << endl;

    VacDB db(101, [](string key) -> unsigned int {
        return static_cast<unsigned int>(hash<string>{}(key) % 1000);
    }, LINEAR);

    int countToInsert = 1000;  // Adjust based on your hash table's capacity and expected load factor
    bool allInserted = true;
    for (int i = 0; i < countToInsert; i++) {
        string key = "HighVolume" + to_string(i);
        Patient newPatient(key, 1000 + i);
        bool inserted = db.insert(newPatient);
        allInserted &= inserted;
        if (!inserted) {
            cout << "Insertion failed at index " << i << " with key " << key << endl;
            break;
        }
    }

    if (allInserted)
        cout << "All " << countToInsert << " items inserted successfully." << endl;
    else
        cout << "Insertion test failed." << endl;
}




void Tester::testConsistencyPostRehash() {
    cout << "Testing Consistency Post-Rehash..." << endl;
    VacDB db(50, [](string key) -> unsigned int {
        return static_cast<unsigned int>(hash<string>{}(key) % 100);
    }, LINEAR);

    vector<pair<string, int>> patients;
    for (int i = 0; i < 100; i++) {
        string key = "PostRehash" + to_string(i);
        patients.emplace_back(key, MINID + i);
        db.insert(Patient(key, MINID + i));
    }

    // Trigger rehash manually or by conditions
    db.changeProbPolicy(DOUBLEHASH);  // Assuming this triggers rehash

    bool allConsistent = true;
    for (auto& [key, id] : patients) {
        const Patient& found = db.getPatient(key, id);
        allConsistent &= (found.getSerial() == id);
    }
    cout << "Data Consistency Post-Rehash Test: " << (allConsistent ? "PASS" : "FAIL") << endl;
}

void Tester::testInsertionDuplicates() {
    cout << "Testing Insertion of Duplicates..." << endl;

    VacDB db(101, [](string key) -> unsigned int {
        return static_cast<unsigned int>(hash<string>{}(key) % 101);
    }, LINEAR);

    string key = "Alice";
    Patient firstPatient(key, 1001);
    Patient duplicatePatient(key, 1001); // Same key, same serial, should not be inserted

    bool firstInsert = db.insert(firstPatient);
    bool secondInsert = db.insert(duplicatePatient); // Expected to fail

    if (firstInsert && !secondInsert) {
        cout << "Duplicate Insertion Test: PASS" << endl;
    } else {
        cout << "Duplicate Insertion Test: FAIL" << endl;
    }
}


unsigned int hashCode(const string str);

string namesDB[6] = {"john", "serina", "mike", "celina", "alexander", "jessica"};

unsigned int hashCode(const string str) {
   unsigned int val = 0 ;
   const unsigned int thirtyThree = 33 ;  // magic number from textbook
   for (unsigned int i = 0 ; i < str.length(); i++)
      val = val * thirtyThree + str[i] ;
   return val ;
}

void Tester::testInsertionBoundarySerialNumbers() {
    cout << "Testing Insertion at Serial Number Boundaries..." << endl;

    VacDB db(101, [](string key) -> unsigned int {
        return static_cast<unsigned int>(hash<string>{}(key) % 101);
    }, LINEAR);

    // Test the boundaries of the serial number
    Patient patientAtMin("BoundaryMin", MINID);
    Patient patientAtMax("BoundaryMax", MAXID);

    bool insertedMin = db.insert(patientAtMin);
    bool insertedMax = db.insert(patientAtMax);

    if (insertedMin && insertedMax) {
        cout << "Boundary Serial Numbers Insertion Test: PASS" << endl;
    } else {
        cout << "Boundary Serial Numbers Insertion Test: FAIL" << endl;
    }
}


void Tester::testRemoveAcrossTables() {
    cout << "Testing Remove Operation Across Current and Old Tables..." << endl;

    VacDB db(10, [](string key) -> unsigned int { // Small size to trigger rehash
        return static_cast<unsigned int>(hash<string>{}(key) % 10);
    }, LINEAR);

    // Fill the table to trigger rehashing
    for (int i = 0; i < 15; i++) {
        db.insert(Patient("Patient" + to_string(i), 2000 + i));
    }

    // Insert more items to start rehashing
    bool removedFromOldTable = false;
    // Remove items that should be in the old table
    for (int i = 0; i < 5; i++) {
        if (db.remove(Patient("Patient" + to_string(i), 2000 + i))) {
            removedFromOldTable = true;
        }
    }

    cout << "Remove Across Tables Test: " << (removedFromOldTable ? "PASS" : "FAIL") << endl;
}



int main() {
    vector<Patient> dataList;
    Random RndID(MINID,MAXID);
    Random RndName(0,5);// selects one from the namesDB array
    Random RndQuantity(0,50);
    VacDB db(MINPRIME, hashCode, DOUBLEHASH);
    bool result = true;

    Tester::testInsertion();
    Tester::testFindError(db);
    Tester::testFindNonColliding();
    Tester::testFindColliding();
    Tester::testRemoveNonColliding();
    Tester::testRemoveColliding();
    Tester::testRehashTriggerOnInsertion();
    Tester::testRehashCompletionOnInsertion();
    Tester::testRehashTriggerOnDeletion();
    Tester::testRehashCompletionOnDeletion();
    Tester::testHighVolumeInsertion();
    Tester::testConsistencyPostRehash();
    Tester::testInsertionDuplicates();
    Tester::testInsertionBoundarySerialNumbers();
    Tester::testRemoveNonExistent();
    Tester::testRemoveAcrossTables();



    return 0;
}