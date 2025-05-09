//+--------------------------------------------------------------------------
//
// File:        PatternCyclicCA.h
//
// NightDriverStrip - (c) 2025 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of the NightDriver software project.
//
//    NightDriver is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    NightDriver is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTlichkeit AND FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
//
// Description:
//
//   Implements a 2D Cyclic Cellular Automata pattern, including a
//   Greenberg-Hastings variant, based on a JavaScript implementation.
//   This file combines the header and implementation for simpler
//   integration into projects following this pattern.
//
//
//---------------------------------------------------------------------------

#ifndef PatternCyclicCA_H
#define PatternCyclicCA_H

// Define the possible modes for the Cellular Automata
enum class CAMode
{
    GreenbergHastings,
    Cyclic
};

#define RUTGER 0

class PatternCyclicCA : public LEDStripEffect
{
private:
#if RUTGER
    // Define the setting specifications for the UI
    // This requires a matching INIT_EFFECT_SETTING_SPECS() in effects.cpp or linker errors will ensue
    DECLARE_EFFECT_SETTING_SPECS(mySettingSpecs);
#endif
    // Two buffers for the cellular automaton state
    std::vector<std::vector<uint8_t>> m_buffer1;
    std::vector<std::vector<uint8_t>> m_buffer2;

    // Pointers to the current and next generation buffers
    // Swapping these pointers is faster than copying vector contents
    std::vector<std::vector<uint8_t>>* m_pb1; // Pointer to the previous generation buffer
    std::vector<std::vector<uint8_t>>* m_pb2; // Pointer to the current generation buffer

    // Pattern parameters, exposed as UI settings
    uint8_t m_numStates = 24;      // Number of possible states for each cell
    int m_speed = 60;              // Delay in milliseconds between generations
    unsigned long m_lifetime = 10000; // How long a pattern runs before re-seeding (0 = forever)
    float m_excited = 0.03;        // Probability of a cell starting in the 'excited' state (for GH)
    float m_refractory = 0.64;     // Probability of a cell starting in a 'refractory' state (for GH)
    uint8_t m_threshold = 1;       // Minimum number of neighbors in the 'next' state to trigger a transition
    CAMode m_mode = CAMode::GreenbergHastings; // Use enum class for mode

    // Function pointer to the current generation calculation method
    void (PatternCyclicCA::*m_calcNextGen)();

    uint8_t m_nextVal = 1;         // The 'target' state for neighbors to trigger a transition
    unsigned long m_frameTimer = 0;    // Accumulator for the generation timer
    unsigned long m_patternTimer = 0;  // Accumulator for the pattern lifetime timer
    unsigned long m_lastDrawTime = 0;  // Time of the last Draw call, for calculating delta

    int m_sum = 0; // Tracks changes in the last generation; used to detect if pattern died

    // Helper functions for toroidal wrapping (wrapping around the matrix edges)
    int WrapX(int x) const { return (x % MATRIX_WIDTH + MATRIX_WIDTH) % MATRIX_WIDTH; }
    int WrapY(int y) const { return (y % MATRIX_HEIGHT + MATRIX_HEIGHT) % MATRIX_HEIGHT; }

    // Allocate and initialize the frame buffers
    void AllocateFrameBuffers()
    {
        // Resize the vectors to the matrix dimensions
        m_buffer1.resize(MATRIX_WIDTH, std::vector<uint8_t>(MATRIX_HEIGHT));
        m_buffer2.resize(MATRIX_WIDTH, std::vector<uint8_t>(MATRIX_HEIGHT));

        // Set initial buffer pointers
        m_pb1 = &m_buffer1;
        m_pb2 = &m_buffer2;
    }

    // Swap the roles of the previous and current buffers
    void SwapBuffers()
    {
        std::swap(m_pb1, m_pb2);
    }

    // Count neighbors in the 'm_nextVal' state using a 4-neighbor (von Neumann) neighborhood
    int SumNeighborhood4(int x, int y, const std::vector<std::vector<uint8_t>>& buffer) const
    {
        int xm = WrapX(x - 1); // Use helper function
        int xp = WrapX(x + 1); // Use helper function
        int ym = WrapY(y - 1); // Use helper function
        int yp = WrapY(y + 1); // Use helper function

        int count = 0;
        if (buffer[x][ym] == m_nextVal) count++;
        if (buffer[x][yp] == m_nextVal) count++;
        if (buffer[xm][y] == m_nextVal) count++;
        if (buffer[xp][y] == m_nextVal) count++;

        return count;
    }

    // Count neighbors in the 'm_nextVal' state using an 8-neighbor (Moore) neighborhood
    int SumNeighborhood8(int x, int y, const std::vector<std::vector<uint8_t>>& buffer) const
    {
        int xm = WrapX(x - 1); // Use helper function
        int xp = WrapX(x + 1); // Use helper function
        int ym = WrapY(y - 1); // Use helper function
        int yp = WrapY(y + 1); // Use helper function

        int count = 0;
        if (buffer[xm][ym] == m_nextVal) count++;
        if (buffer[x][ym] == m_nextVal) count++;
        if (buffer[xp][ym] == m_nextVal) count++;
        if (buffer[xm][y] == m_nextVal) count++;
        if (buffer[xp][y] == m_nextVal) count++;
        if (buffer[xm][yp] == m_nextVal) count++;
        if (buffer[x][yp] == m_nextVal) count++;
        if (buffer[xp][yp] == m_nextVal) count++;

        return count;
    }

    // Calculate the next generation using the Greenberg-Hastings rules.
    void DoGenerationGH()
    {
        SwapBuffers();

        m_nextVal = 1; // For GH, we look for neighbors in state 1 (excited)

        int current_sum = 0; // Use a local sum to track changes in this generation

        for (int y = 0; y < MATRIX_HEIGHT; y++)
        {
            for (int x = 0; x < MATRIX_WIDTH; x++)
            {
                uint8_t currentState = (*m_pb1)[x][y];

                if (currentState == 0)
                {
                    // If cell is in state 0 (susceptible), check neighbors in state 1 (excited)
                    int neighbors = SumNeighborhood4(x, y, *m_pb1);
                    (*m_pb2)[x][y] = (neighbors >= m_threshold) ? 1 : 0;
                }
                else
                {
                    // If cell is in state > 0, advance to the next state (refractory period)
                    (*m_pb2)[x][y] = (currentState + 1) % m_numStates;
                }
                 // Track if the cell state changed for the 'm_sum' variable
                if ((*m_pb2)[x][y] != currentState)
                {
                     current_sum++;
                }
            }
        }
        m_sum = current_sum; // Update the member variable m_sum
    }

    // Calculate the next generation using the classic Cyclic CA rules.
    void DoGenerationCCA()
    {
        SwapBuffers();

        int current_sum = 0;

        for (int y = 0; y < MATRIX_HEIGHT; y++)
        {
            for (int x = 0; x < MATRIX_WIDTH; x++)
            {
                uint8_t currentState = (*m_pb1)[x][y];
                // The state the current cell *wants* to transition to
                m_nextVal = (currentState + 1) % m_numStates;

                // Count neighbors in the 'm_nextVal' state using an 8-neighbor neighborhood.
                int neighbors = SumNeighborhood8(x, y, *m_pb1);

                // Apply the Cyclic CA rule: if enough neighbors are in the 'm_nextVal' state,
                // the current cell transitions to 'm_nextVal'. Otherwise, it stays in its current state.
                (*m_pb2)[x][y] = (neighbors >= m_threshold) ? m_nextVal : currentState;

                // Track if the cell state changed for the 'm_sum' variable
                if ((*m_pb2)[x][y] != currentState)
                {
                     current_sum++;
                }
            }
        }
        m_sum = current_sum; // Update the member variable m_sum
    }

    // Initialize the grid with random states for classic Cyclic CA.
    void SeedCCA()
    {
        // Set all cells to a random activation level between 0 and m_numStates - 1
        for (int y = 0; y < MATRIX_HEIGHT; y++)
        {
            for (int x = 0; x < MATRIX_WIDTH; x++)
            {
                uint8_t randomState = random(m_numStates);
                m_buffer1[x][y] = randomState;
                m_buffer2[x][y] = randomState; // Initialize both buffers the same
            }
        }
        // Set up buffer pointers
        m_pb1 = &m_buffer1;
        m_pb2 = &m_buffer2;
        m_sum = 1; // Ensure m_sum is non-zero to prevent immediate re-seeding
    }

    // Initialize the grid with random states for Greenberg-Hastings CA.
    void SeedGH(float probX, float probR)
    {
        // Zero arrays initially using std::fill and range-based for loops
        for (auto& row : m_buffer1)
        {
            std::fill(row.begin(), row.end(), 0);
        }
        for (auto& row : m_buffer2)
        {
            std::fill(row.begin(), row.end(), 0);
        }

        // Distribute excited cells (state 1) based on probability
        int numExcited = floor(MATRIX_WIDTH * MATRIX_HEIGHT * probX);
        int count = 0;
        while (count < numExcited)
        {
            int x = random(MATRIX_WIDTH);
            int y = random(MATRIX_HEIGHT);
            // Only place excited cells in empty (state 0) spots
            if (m_buffer2[x][y] == 0)
            {
                m_buffer2[x][y] = 1; // State 1 is the excited state
                count++;
            }
        }

        // Distribute refractory cells (state 2 to m_numStates-1) based on probability
        int numRefractory = floor(MATRIX_WIDTH * MATRIX_HEIGHT * probR);
        count = 0;
        while (count < numRefractory)
        {
            int x = random(MATRIX_WIDTH);
            int y = random(MATRIX_HEIGHT);
            // Only place refractory cells in empty (state 0) spots
            if (m_buffer2[x][y] == 0)
            {
                // Assign a random refractory state between 2 and m_numStates - 1
                // Ensure m_numStates is at least 3 for refractory states to exist
                if (m_numStates > 2)
                {
                     m_buffer2[x][y] = 2 + random(m_numStates - 2);
                     count++;
                }
                else
                {
                    // If m_numStates is 2, only states 0 and 1 exist, no refractory states
                    break;
                }
            }
        }

        // Set up buffer pointers
        m_pb1 = &m_buffer1;
        m_pb2 = &m_buffer2;
        m_sum = 1; // Ensure m_sum is non-zero to prevent immediate re-seeding
    }

    // Master seeding function, calls the appropriate seed function based on mode.
    void SeedCA()
    {
        if (m_mode == CAMode::Cyclic)
        {
            SeedCCA();
        }
        else // Greenberg-Hastings mode
        {
            SeedGH(m_excited, m_refractory);
        }
    }

    // Map a cell's state value to a color
    CRGB ColorFromCellState(uint8_t state) const
    {
        if (m_numStates <= 1) return CRGB::Black; // Avoid division by zero or mapping issues
        // Map the state (0 to m_numStates-1) to a hue value (0 to 255)
        uint8_t hue = map(state, 0, m_numStates - 1, 0, 255);
        // Use full saturation and value for now. The original JS 'wave(state)'
        // might have modulated brightness, but mapping state directly to hue is a common
        // visualization for CA and matches the JS hsv call's first parameter.
        return CHSV(hue, 255, 255);
    }


protected:
#if RUTGER
    // Define the specifications for the UI settings sliders.
    EffectSettingSpecs* FillSettingSpecs() override
    {
        return &mySettingSpecs;
    }

    // Handle incoming setting changes from the UI.
    bool SetSetting(const String& name, const String& value) override
    {
        // Use helper macros provided by the framework to handle common types.
        RETURN_IF_SET(name, NAME_OF(m_speed), m_speed, value);
        RETURN_IF_SET(name, NAME_OF(m_lifetime), m_lifetime, value);
        RETURN_IF_SET(name, NAME_OF(m_threshold), m_threshold, value);
        RETURN_IF_SET(name, NAME_OF(m_numStates), m_numStates, value);
        RETURN_IF_SET(name, NAME_OF(m_excited), m_excited, value);
        RETURN_IF_SET(name, NAME_OF(m_refractory), m_refractory, value);

        // Handle the 'm_mode' setting separately as it affects the generation logic and defaults
        if (name.equalsIgnoreCase(NAME_OF(m_mode)))
        {
            // Parse the incoming string value for the mode
            bool newModeBool = false;
            if (value.equalsIgnoreCase("true") || value.equalsIgnoreCase("1")) {
                newModeBool = true;
            } else if (value.equalsIgnoreCase("false") || value.equalsIgnoreCase("0")) {
                newModeBool = false;
            } else {
                // Handle potential string names for modes if needed, e.g., "Greenberg-Hastings", "Cyclic"
                // For now, assume boolean or integer string representation
                return false; // Value not recognized
            }

            CAMode newMode = newModeBool ? CAMode::Cyclic : CAMode::GreenbergHastings;

            if (m_mode != newMode) {
                m_mode = newMode; // Update the mode

                // Update the function pointer based on the new mode.
                m_calcNextGen = (m_mode == CAMode::Cyclic) ? &PatternCyclicCA::DoGenerationCCA : &PatternCyclicCA::DoGenerationGH;

                // Reset default values for some parameters based on the mode change,
                // mirroring the logic in the original JavaScript sliderMode function.
                if (m_mode == CAMode::GreenbergHastings) // Switched to Greenberg-Hastings mode
                {
                    m_threshold = 1;
                    m_numStates = 24;
                    m_excited = 0.03;
                    m_refractory = 0.64;
                }
                else // Switched to Cyclic CA mode
                {
                    m_threshold = 3;
                    m_numStates = 3;
                    // These probabilities are primarily for GH, reset them for CCA
                    m_excited = 0.0;
                    m_refractory = 0.0;
                }
                // Re-seed the pattern with the new mode and parameters.
                SeedCA();
                // Reset timers to start the new pattern immediately
                m_patternTimer = 0;
                m_frameTimer = 0;
            }
            return true; // Setting was handled
        }


        // If the setting name doesn't match any in this class, pass it to the base class.
        return LEDStripEffect::SetSetting(name, value);
    }

    // Implement SerializeSettingsToJSON to save the effect's settings.
    bool SerializeSettingsToJSON(JsonObject& jsonObject) override
    {
        // Serialize base class settings first.
        auto jsonDoc = CreateJsonDocument();
        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeSettingsToJSON(root); // Base class method

        // Serialize our specific settings.
        // Using NAME_OF macro for consistency with SetSetting and FillSettingSpecs
        jsonDoc[NAME_OF(m_numStates)] = m_numStates;
        jsonDoc[NAME_OF(m_speed)] = m_speed;
        jsonDoc[NAME_OF(m_lifetime)] = m_lifetime;
        jsonDoc[NAME_OF(m_excited)] = m_excited;
        jsonDoc[NAME_OF(m_refractory)] = m_refractory;
        jsonDoc[NAME_OF(m_threshold)] = m_threshold;
        // Serialize the enum as a boolean for compatibility with the existing JSON structure
        jsonDoc[NAME_OF(m_mode)] = (m_mode == CAMode::Cyclic);

        // Merge our settings into the main JSON object.
        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }
#endif

public:
    // Constructor
    PatternCyclicCA() : LEDStripEffect(EFFECT_MATRIX_CYCLIC_CA, "Cyclic CA")
    {
        // Initialize member variables to default values
        m_numStates = 24;
        m_speed = 60;
        m_lifetime = 10000;
        m_excited = 0.03;
        m_refractory = 0.64;
        m_threshold = 1;
        m_mode = CAMode::GreenbergHastings; // Default to Greenberg-Hastings mode using enum
        m_calcNextGen = &PatternCyclicCA::DoGenerationGH; // Set function pointer.
        m_nextVal = 1;
        m_frameTimer = 0;
        m_patternTimer = 0;
        m_sum = 0;
        m_lastDrawTime = 0;
        m_pb1 = nullptr; // Initialize buffer pointers
        m_pb2 = nullptr;
    }

    // Constructor for deserialization from JSON
    PatternCyclicCA(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
        // Deserialize settings from JSON, using default values if keys are not found
        m_numStates = jsonObject["numStates"] | m_numStates;
        m_speed = jsonObject["speed"] | m_speed;
        m_lifetime = jsonObject["lifetime"] | m_lifetime;
        m_excited = jsonObject["excited"] | m_excited;
        m_refractory = jsonObject["refractory"] | m_refractory;
        m_threshold = jsonObject["threshold"] | m_threshold;

        // Deserialize the mode from boolean to enum
        bool deserializedModeBool = jsonObject["mode"] | false;
        m_mode = deserializedModeBool ? CAMode::Cyclic : CAMode::GreenbergHastings;

        // Set the correct generation function based on deserialized mode.
        m_calcNextGen = (m_mode == CAMode::Cyclic) ? &PatternCyclicCA::DoGenerationCCA : &PatternCyclicCA::DoGenerationGH;

        // Initialize other member variables
        m_nextVal = 1; // Default, might be updated in DoGeneration functions
        m_frameTimer = 0;
        m_patternTimer = 0;
        m_sum = 0;
        m_lastDrawTime = 0;
        m_pb1 = nullptr; // Initialize buffer pointers
        m_pb2 = nullptr;
    }

    // Virtual destructor - definition inlined
    virtual ~PatternCyclicCA()
    {
        // Destructor body (can be empty if no specific cleanup is needed beyond member destruction)
    }

    // Called once when the effect is started.
    void Start() override
    {
        // Allocate buffers based on the current matrix dimensions.
        AllocateFrameBuffers();
        // Seed the CA with an initial random state.
        SeedCA();
        // Reset timers
        m_patternTimer = 0;
        m_frameTimer = 0;
        // Initialize lastDrawTime for delta calculation
        m_lastDrawTime = millis();
    }

    // Called repeatedly to draw the next frame.
    void Draw() override
    {
        unsigned long currentTime = millis();
        // Calculate the time elapsed since the last Draw call
        unsigned long delta = currentTime - m_lastDrawTime;
        m_lastDrawTime = currentTime;

        // Accumulate time for frame and pattern timers
        m_frameTimer += delta;
        m_patternTimer += delta;

        // Check if the pattern has died (no state changes in the last generation)
        // or if the pattern lifetime has been reached (if lifetime is not 0)
        // Re-seed the pattern if either condition is met.
        // Added check for m_lastDrawTime > 0 to prevent re-seeding immediately on start
        if ((m_sum == 0 && m_lastDrawTime > 0) || (m_lifetime > 0 && (m_patternTimer >= m_lifetime)))
        {
            SeedCA();
            m_patternTimer = 0; // Reset pattern timer
            m_frameTimer = 0;   // Reset frame timer
            // Note: m_sum is reset within SeedCA
        }

        // Check if enough time has passed to calculate the next generation.
        if (m_frameTimer >= m_speed)
        {
            // Calculate the next generation using the currently selected method (GH or CCA).
            (this->*m_calcNextGen)();
            m_frameTimer = 0; // Reset frame timer
            // Note: m_sum is updated within the generation functions.
        }

        // Render the current state of the CA grid to the matrix.
        g()->fillScreen(BLACK16); // Clear the screen before drawing the new frame

        for (int y = 0; y < MATRIX_HEIGHT; y++)
        {
            for (int x = 0; x < MATRIX_WIDTH; x++)
            {
                // Get the cell state from the current buffer (m_pb2).
                uint8_t cellState = (*m_pb2)[x][y];
                // Map the state to a color.
                CRGB pixelColor = ColorFromCellState(cellState);
                // Draw the pixel on the matrix.
                g()->setPixel(x, y, pixelColor);
            }
        }
    }

    // Specify the desired base frame rate (actual rate controlled by 'speed')
    virtual size_t DesiredFramesPerSecond() const override
    {
        return 60; // A reasonable base rate for the effect runner
    }

    // Indicate that this effect manages its own buffers
    bool RequiresDoubleBuffering() const override
    {
        return false;
    }

    // Serialize the entire effect state to JSON (for saving/loading effect instances).
    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        // Serialize base class data first.
        auto jsonDoc = CreateJsonDocument();
        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root); // Base class method

        // Serialize our specific settings (using short keys for efficiency in JSON)
        jsonDoc["ns"] = m_numStates;
        jsonDoc["sp"] = m_speed;
        jsonDoc["lt"] = m_lifetime;
        jsonDoc["ex"] = m_excited;
        jsonDoc["rf"] = m_refractory;
        jsonDoc["th"] = m_threshold;
        // Serialize the enum as a boolean for compatibility with the existing JSON structure
        jsonDoc["mo"] = (m_mode == CAMode::Cyclic);

        // Merge our data into the main JSON object.
        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }
};

// Define the setting specifications for the UI
#if RUTGER
INIT_EFFECT_SETTING_SPECS(PatternCyclicCA::mySettingSpecs) = {
    // NAME_OF(variable), "Label", "Description", SettingSpec::SettingType, min, max, step
    EffectSettingSpecs::AddSettingSpec(NAME_OF(m_speed), "Speed", "Milliseconds per frame", SettingSpec::SettingType::Int, 1, 200, 1),
    EffectSettingSpecs::AddSettingSpec(NAME_OF(m_lifetime), "Lifetime", "Pattern lifetime in ms (0=forever)", SettingSpec::SettingType::UInt, 0, 60000, 1000),
    EffectSettingSpecs::AddSettingSpec(NAME_OF(m_threshold), "Threshold", "Neighbors to advance state", SettingSpec::SettingType::UInt8, 1, 8, 1),
    EffectSettingSpecs::AddSettingSpec(NAME_OF(m_numStates), "States", "Number of cell states", SettingSpec::SettingType::UInt8, 2, 32, 1),
    EffectSettingSpecs::AddSettingSpec(NAME_OF(m_excited), "Excited %", "Initial excited cell percentage (GH)", SettingSpec::SettingType::Float, 0.0, 0.2, 0.01),
    EffectSettingSpecs::AddSettingSpec(NAME_OF(m_refractory), "Refractory %", "Initial refractory cell percentage (GH)", SettingSpec::SettingType::Float, 0.0, 0.8, 0.01),
    // Keep as Bool type, assuming the UI handles mapping bool toggle to the two modes
    EffectSettingSpecs::AddSettingSpec(NAME_OF(m_mode), "Mode", "0=Greenberg-Hastings, 1=Cyclic CA", SettingSpec::SettingType::Bool),
};
#endif

#endif // PatternCyclicCA_H