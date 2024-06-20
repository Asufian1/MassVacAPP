// CMSC 341 - Spring 2024 - Project 4
#include "vacdb.h"

/**
 * Name: Constructor
 * Desc: Initializes a VacDB object with a specific initial size, hash function, and collision handling method.
 *       The size is adjusted to the nearest valid prime number to optimize hash distribution.
 * Preconditions: The size should be a non-negative integer.
 * Postconditions: A hash table is initialized with capacity set to a valid prime number.
 *                 If the specified size is not within the valid range, it is adjusted to the nearest prime number within the range.
 */
VacDB::VacDB(int size, hash_fn hash, prob_t probing)
    : m_hash(hash), m_currProbing(probing), m_currentTable(nullptr),
      m_currentCap(0), m_currentSize(0), m_currNumDeleted(0),
      m_oldTable(nullptr), m_oldCap(0), m_oldSize(0), m_oldNumDeleted(0),
      m_transferIndex(0) {

    // Adjust the initial size to a valid prime number
    if (size < MINPRIME) {
        m_currentCap = MINPRIME;
    } else if (size > MAXPRIME) {
        m_currentCap = MAXPRIME;
    } else if (!isPrime(size)) {
        m_currentCap = findNextPrime(size);
    } else {
        m_currentCap = size;
    }

    // Allocate memory for the hash table
    m_currentTable = new Patient*[m_currentCap];
    for (int i = 0; i < m_currentCap; i++) {
        m_currentTable[i] = nullptr;
    }
}



/**
 * Name: Destructor
 * Desc: Cleans up all dynamic memory associated with the VacDB, ensuring no memory leaks.
 *       This includes deleting all patient pointers and the array that holds them.
 * Preconditions: The hash table has been initialized.
 * Postconditions: All memory allocated to the hash table and its elements is freed, and the table is left in an unusable state.
 */
VacDB::~VacDB() {
    for (int i = 0; i < m_currentCap; ++i) {
        delete m_currentTable[i];
        m_currentTable[i] = nullptr;
    }
    delete[] m_currentTable;
    m_currentTable = nullptr;

    if (m_oldTable) {
        for (int i = 0; i < m_oldCap; ++i) {
            delete m_oldTable[i];
            m_oldTable[i] = nullptr;
        }
        delete[] m_oldTable;
        m_oldTable = nullptr;
    }
}


/**
 * Name: changeProbPolicy
 * Desc: Changes the probing policy of the hash table and triggers a rehash to apply the new policy.
 * Preconditions: The hash table is initialized and the provided policy is a valid prob_t enumeration value.
 * Postconditions: The probing policy is updated and a rehash is performed if necessary.
 */
void VacDB::changeProbPolicy(prob_t policy) {
    // Store the new policy in m_newPolicy
    m_newPolicy = policy;

    //rehash(); // Ensure that the rehash function uses m_newPolicy for the new table
}



/**
 * Name: insert
 * Desc: Attempts to insert a new patient into the hash table. If the patient already exists, or the serial number is out of the valid range, the insertion will fail.
 * Preconditions: The hash table is initialized. The patient's serial number must be within the defined valid range.
 * Postconditions: If successful, the patient is added to the hash table. If the table reaches a high load factor or has too many deleted entries, a rehash may be triggered.
 */
bool VacDB::insert(Patient patient) {
    if (patient.getSerial() < MINID || patient.getSerial() > MAXID) {
        return false;  // Serial number out of range
    }

    // Check for existing patient to avoid duplicates
    if (getPatient(patient.getKey(), patient.getSerial()).getUsed()) {
        return false;  // Patient already exists
    }

    unsigned int bucket = m_hash(patient.getKey()) % m_currentCap;
    unsigned int step = 0;
    unsigned int index;

    while (step < m_currentCap) {
        index = bucket;  // Start with the initial hash index
        if (m_currProbing == DOUBLEHASH) {
            index = (bucket + step * (11 - (m_hash(patient.getKey()) % 11))) % m_currentCap;
        } else if (m_currProbing == QUADRATIC) {
            index = (bucket + step * step) % m_currentCap;
        } else if (m_currProbing == LINEAR) {
            index = (bucket + step) % m_currentCap;
        }

        // Check if the bucket is empty or marked as deleted
        if (m_currentTable[index] == nullptr || !m_currentTable[index]->getUsed()) {
            if (m_currentTable[index] == nullptr) {
                m_currentTable[index] = new Patient();  // Allocate new if it was nullptr
            }
            *m_currentTable[index] = patient;  // Copy assignment
            m_currentTable[index]->setUsed(true);
            m_currentSize++;
            // Check if rehashing is needed
            if (lambda() > 0.5 || deletedRatio() > 0.8) {
                rehash();  // Implement the rehash function separately
            }
            return true;
        }
        step++;
    }
    return false;  // Table full
}


/**
 * Name: rehash
 * Desc: Performs rehashing of the hash table to a new table with double the current prime capacity.
 *       This method is typically called automatically when load factor or deleted item thresholds are exceeded.
 * Preconditions: The hash table is initialized and needs resizing due to load factors or deletion thresholds.
 * Postconditions: The hash table's capacity is increased, and all existing, non-deleted entries are transferred to the new table.
 */
void VacDB::rehash() {
    int newSize = findNextPrime(m_currentCap * 2);
    Patient** newTable = new Patient*[newSize] {};
    int newTableSize = 0;

    for (int i = 0; i < m_currentCap; i++) {
        if (m_currentTable[i] != nullptr && m_currentTable[i]->getUsed()) {
            unsigned int bucket = m_hash(m_currentTable[i]->getKey()) % newSize;
            unsigned int step = 0;
            unsigned int index;

            while (step < newSize) {
                index = (bucket + step * (11 - (m_hash(m_currentTable[i]->getKey()) % 11))) % newSize;
                if (newTable[index] == nullptr) {
                    newTable[index] = m_currentTable[i];
                    newTableSize++;
                    break;
                }
                step++;
            }
        }
    }

    delete[] m_currentTable;  // Free old table
    m_currentTable = newTable;
    m_currentCap = newSize;
    m_currentSize = newTableSize;
}





/**
 * Name: remove
 * Desc: Attempts to remove a specified patient from the hash table based on their key.
 * Preconditions: The hash table is initialized and contains at least one entry.
 * Postconditions: If the patient is found, they are marked as not used. The method returns true if successful, false otherwise.
 */
bool VacDB::remove(Patient patient) {
    unsigned int index = m_hash(patient.getKey()) % m_currentCap;
    unsigned int step = 0;
    while (step < m_currentCap) {
        int probeIndex = (index + step * (11 - (m_hash(patient.getKey()) % 11))) % m_currentCap;
        Patient* entry = m_currentTable[probeIndex];
        if (entry != nullptr && entry->getUsed() && entry->getKey() == patient.getKey()) {
            entry->setUsed(false); // Mark the entry as not used
            m_currNumDeleted++;    // Increment the count of deleted entries
            m_currentSize--;       // Decrement the current size
            return true;
        }
        step++;
    }
    return false;
}



/**
 * Name: getPatient
 * Desc: Retrieves a patient based on their name and serial number.
 * Preconditions: The hash table is initialized.
 * Postconditions: Returns the patient if found. If no matching patient is found, returns an empty Patient object.
 */
const Patient VacDB::getPatient(string name, int serial) const {
    for (int i = 0; i < m_currentCap; i++) {
        Patient* entry = m_currentTable[i];
        if (entry != nullptr && entry->getUsed() && entry->getKey() == name && entry->getSerial() == serial) {
            return *entry;
        }
    }
    return Patient(); // Return an empty patient if not found
}


/**
 * Name: updateSerialNumber
 * Desc: Updates the serial number of a specific patient in the hash table.
 * Preconditions: The hash table is initialized and contains the patient to be updated.
 * Postconditions: If the patient is found, their serial number is updated. Returns true if successful, false otherwise.
 */
bool VacDB::updateSerialNumber(Patient patient, int serial) {
    unsigned int index = m_hash(patient.getKey()) % m_currentCap;
    unsigned int step = 0;
    while (step < m_currentCap) {
        int probeIndex = (index + step * (11 - (m_hash(patient.getKey()) % 11))) % m_currentCap;
        Patient* entry = m_currentTable[probeIndex];
        if (entry != nullptr && entry->getUsed() && entry->getKey() == patient.getKey()) {
            entry->setSerial(serial);
            return true;
        }
        step++;
    }
    return false;
}


/**
 * Name: lambda
 * Desc: Calculates the current load factor of the hash table, defined as the ratio of used slots to total capacity.
 * Preconditions: The hash table is initialized.
 * Postconditions: Returns the current load factor as a float.
 */
float VacDB::lambda() const {
    return float(m_currentSize) / float(m_currentCap);
}


/**
 * Name: deletedRatio
 * Desc: Calculates the ratio of deleted slots to the total number of slots in the hash table.
 * Preconditions: The hash table is initialized.
 * Postconditions: Returns the ratio of deleted slots as a float.
 */
float VacDB::deletedRatio() const {
    return float(m_currNumDeleted) / float(m_currentSize);
}

void VacDB::dump() const {
    cout << "Dump for the current table: " << endl;
    if (m_currentTable != nullptr)
        for (int i = 0; i < m_currentCap; i++) {
            cout << "[" << i << "] : " << m_currentTable[i] << endl;
        }
    cout << "Dump for the old table: " << endl;
    if (m_oldTable != nullptr)
        for (int i = 0; i < m_oldCap; i++) {
            cout << "[" << i << "] : " << m_oldTable[i] << endl;
        }
}

/**
 * Name: isPrime
 * Desc: Determines if a given number is prime.
 * Preconditions: An integer number is provided.
 * Postconditions: Returns true if the number is prime, false otherwise.
 */
bool VacDB::isPrime(int number){
    bool result = true;
    for (int i = 2; i <= number / 2; ++i) {
        if (number % i == 0) {
            result = false;
            break;
        }
    }
    return result;
}


/**
 * Name: findNextPrime
 * Desc: Finds the next prime number following a given number, within a specified range.
 * Preconditions: An integer current is provided, representing the starting point for the search.
 * Postconditions: Returns the next prime number following 'current', unless it exceeds the maximum allowed prime, in which case the maximum is returned.
 */
int VacDB::findNextPrime(int current){
    //we always stay within the range [MINPRIME-MAXPRIME]
    //the smallest prime starts at MINPRIME
    if (current < MINPRIME) current = MINPRIME-1;
    for (int i=current; i<MAXPRIME; i++) { 
        for (int j=2; j*j<=i; j++) {
            if (i % j == 0) 
                break;
            else if (j+1 > sqrt(i) && i != current) {
                return i;
            }
        }
    }
    //if a user tries to go over MAXPRIME
    return MAXPRIME;
}


ostream& operator<<(ostream& sout, const Patient* patient ) {
    if ((patient != nullptr) && !(patient->getKey().empty()))
        sout << patient->getKey() << " (" << patient->getSerial() << ", "<< patient->getUsed() <<  ")";
    else
        sout << "";
  return sout;
}


/**
 * Name: operator==
 * Desc: Overloads the equality operator for Patient objects, comparing based on the patient's key and serial number.
 * Preconditions: Two Patient objects are provided.
 * Postconditions: Returns true if both objects have the same key and serial number, false otherwise.
 */
bool operator==(const Patient& lhs, const Patient& rhs){
    // since the uniqueness of an object is defined by name and serial number
    // the equality operator considers only those two criteria
    return ((lhs.getKey() == rhs.getKey()) && (lhs.getSerial() == rhs.getSerial()));
}

bool Patient::operator==(const Patient* & rhs){
    // since the uniqueness of an object is defined by name and serial number
    // the equality operator considers only those two criteria
    return ((getKey() == rhs->getKey()) && (getSerial() == rhs->getSerial()));
}

/**
 * Name: getCurrentSize
 * Desc: Returns the number of active entries in the hash table.
 * Preconditions: None.
 * Postconditions: Returns the current number of active entries, excluding any marked as deleted.
 */
int VacDB::getCurrentSize() const {
    return m_currentSize;
}


/**
 * Name: findIndex
 * Desc: Returns the index of the bucket where the specified key is stored, if it exists.
 * Preconditions: The key is a valid string.
 * Postconditions: Returns the index of the bucket containing the key, or -1 if the key is not found.
 */
int VacDB::findIndex(const string& key) const {
    unsigned int hashValue = m_hash(key);
    unsigned int index;

    for (unsigned int step = 0; step < m_currentCap; ++step) {
        if (m_currProbing == DOUBLEHASH) {
            index = (hashValue + step * (11 - (m_hash(key) % 11))) % m_currentCap;
        } else if (m_currProbing == QUADRATIC) {
            index = (hashValue + step * step) % m_currentCap;
        } else if (m_currProbing == LINEAR) {
            index = (hashValue + step) % m_currentCap;
        }

        if (m_currentTable[index] == nullptr) {
            return -1;  // Key is not present.
        } else if (m_currentTable[index]->getUsed() && m_currentTable[index]->getKey() == key) {
            return index;  // Key found.
        }
    }
    return -1;  // Key not found after full probe.
}
