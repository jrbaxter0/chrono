// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Author: Milad Rakhsha, Arman Pazouki, Wei Hu
// =============================================================================

#include <assert.h>
#include <stdlib.h>
#include <ctime>

#include "chrono/physics/ChSystemSMC.h"
#include "chrono/utils/ChUtilsGenerators.h"

#include "chrono_fsi/ChSystemFsi.h"

using namespace chrono;
using namespace chrono::collision;
using namespace chrono::fsi;

// Output directories and settings
const std::string out_dir = GetChronoOutputPath() + "FSI_COMPRESSIBILITY/";
std::string demo_dir;

// Save data as csv files, turn it on to be able to see the results off-line using paraview
bool save_output = true;

// Output frequency
double out_fps = 20;

// Size of the box
double bxDim = 1.0;
double byDim = 1.0;
double bzDim = 1.4;

// Size of the fluid domain
double fxDim = bxDim;
double fyDim = byDim;
double fzDim = 1.0;

// Final simulation time
double t_end = 2.0;

// -----------------------------------------------------------------
// Show command line usage
// -----------------------------------------------------------------
void ShowUsage() {
    std::cout << "usage: ./demo_FSI_Compressibility <json_file>" << std::endl;
}

//------------------------------------------------------------------
// Function to save the paraview files
//------------------------------------------------------------------
void SaveParaViewFilesMBD(ChSystemFsi& sysFSI,
                          ChSystemSMC& sysMBS,
                          int next_frame,
                          double mTime) {
    double frame_time = 1.0 / out_fps;
    static int out_frame = 0;

    if (save_output && std::abs(mTime - (next_frame)*frame_time) < 1e-5) {
        sysFSI.PrintParticleToFile(demo_dir);

        std::cout << "-------------------------------------\n" << std::endl;
        std::cout << "             Output frame:   " << next_frame << std::endl;
        std::cout << "             Time:           " << mTime << std::endl;
        std::cout << "-------------------------------------\n" << std::endl;

        out_frame++;
    }
}

//------------------------------------------------------------------
// Create the objects of the MBD system. Rigid bodies, and if fsi, their
// bce representation are created and added to the systems
//------------------------------------------------------------------
void CreateSolidPhase(ChSystemSMC& sysMBS,
                      ChSystemFsi& sysFSI) {
    auto mysurfmaterial = chrono_types::make_shared<ChMaterialSurfaceSMC>();

    // Set common material Properties
    mysurfmaterial->SetYoungModulus(6e4);
    mysurfmaterial->SetFriction(0.3f);
    mysurfmaterial->SetRestitution(0.2f);
    mysurfmaterial->SetAdhesion(0);
    // Ground body
    auto ground = chrono_types::make_shared<ChBody>();
    ground->SetIdentifier(-1);
    ground->SetBodyFixed(true);
    ground->SetCollide(true);
    ground->GetCollisionModel()->ClearModel();

    auto initSpace0 = sysFSI.GetInitialSpacing();

    // Bottom and top wall
    ChVector<> size_XY(bxDim / 2 + 3 * initSpace0, byDim / 2 + 3 * initSpace0, 2 * initSpace0);
    ChVector<> pos_zp(0, 0, bzDim + 2 * initSpace0);
    ChVector<> pos_zn(0, 0, -2 * initSpace0);

    // Left and right wall
    ChVector<> size_YZ(2 * initSpace0, byDim / 2 + 3 * initSpace0, bzDim / 2);
    ChVector<> pos_xp(bxDim / 2 + initSpace0, 0.0, bzDim / 2 + 1 * initSpace0);
    ChVector<> pos_xn(-bxDim / 2 - 3 * initSpace0, 0.0, bzDim / 2 + 1 * initSpace0);

    // Front and back wall
    ChVector<> size_XZ(bxDim / 2, 2 * initSpace0, bzDim / 2);
    ChVector<> pos_yp(0, byDim / 2 + initSpace0, bzDim / 2 + 1 * initSpace0);
    ChVector<> pos_yn(0, -byDim / 2 - 3 * initSpace0, bzDim / 2 + 1 * initSpace0);

    chrono::utils::AddBoxGeometry(ground.get(), mysurfmaterial, size_XY, pos_zn, QUNIT, true);
    chrono::utils::AddBoxGeometry(ground.get(), mysurfmaterial, size_YZ, pos_xp, QUNIT, true);
    chrono::utils::AddBoxGeometry(ground.get(), mysurfmaterial, size_YZ, pos_xn, QUNIT, true);
    chrono::utils::AddBoxGeometry(ground.get(), mysurfmaterial, size_XZ, pos_yp, QUNIT, true);
    chrono::utils::AddBoxGeometry(ground.get(), mysurfmaterial, size_XZ, pos_yn, QUNIT, true);
    ground->GetCollisionModel()->BuildModel();
    sysMBS.AddBody(ground);

    sysFSI.AddBceBox(ground, pos_zn, QUNIT, size_XY, 12);
    sysFSI.AddBceBox(ground, pos_xp, QUNIT, size_YZ, 23);
    sysFSI.AddBceBox(ground, pos_xn, QUNIT, size_YZ, 23);
    sysFSI.AddBceBox(ground, pos_yp, QUNIT, size_XZ, 13);
    sysFSI.AddBceBox(ground, pos_yn, QUNIT, size_XZ, 13);
}

// =============================================================================

int main(int argc, char* argv[]) {
    // Create a physic system to handle multibody dynamics
    ChSystemSMC sysMBS;

    // Create an FSI system to handle fluid dynamics
    ChSystemFsi sysFSI(sysMBS);

    // Use the default input file or you may enter your input parameters as a command line argument
    std::string inputJson = GetChronoDataFile("fsi/input_json/demo_FSI_Compressibility_Explicit.json");
    if (argc == 1) {
        std::cout << "Use the default JSON file" << std::endl;
    } else if (argc == 2) {
        std::cout << "Use the specified JSON file" << std::endl;
        std::string my_inputJson = std::string(argv[1]);
        inputJson = my_inputJson;
    } else {
        ShowUsage();
        return 1;
    }
    sysFSI.ReadParametersFromFile(inputJson, ChVector<>(bxDim, byDim, bzDim));

    auto initSpace0 = sysFSI.GetInitialSpacing();
    ChVector<> cMin = ChVector<>(-bxDim / 2, -byDim / 2, -bzDim / 2) - ChVector<>(initSpace0 * 20);
    ChVector<> cMax = ChVector<>(bxDim / 2, byDim / 2, bzDim) + ChVector<>(initSpace0 * 10);
    sysFSI.SetBoundaries(cMin, cMax);

    // Setup the output directory for FSI data
    sysFSI.SetFsiOutputDir(demo_dir, out_dir, inputJson);

    // Create an initial box for the terrain patch
    chrono::utils::GridSampler<> sampler(initSpace0);

    // Use a chrono sampler to create a bucket of granular material
    ChVector<> boxCenter(-bxDim / 2 + fxDim / 2, 0 * initSpace0, fzDim / 2 + 1 * initSpace0);
    ChVector<> boxHalfDim(fxDim / 2, fyDim / 2, fzDim / 2);
    std::vector<ChVector<>> points = sampler.SampleBox(boxCenter, boxHalfDim);

    // Add SPH particles from the sampler points to the FSI system
    size_t numPart = points.size();
    for (int i = 0; i < numPart; i++) {
        sysFSI.AddSphMarker(points[i], -1);
    }
    sysFSI.AddRefArray(0, (int)numPart, -1, -1);

    // Create MBD and BCE particles for the solid domain
    CreateSolidPhase(sysMBS, sysFSI);
    sysFSI.SetInitPressure(fzDim);

    // Complete construction of the FSI system
    sysFSI.Initialize();

    SaveParaViewFilesMBD(sysFSI, sysMBS, 0, 0);

    double dT = sysFSI.GetStepSize();
    double time = 0;
    int stepEnd = int(t_end / dT);
    double Global_max_dT = sysFSI.GetMaxStepSize();
    double TIMING_sta = clock();
    for (int tStep = 0; tStep < stepEnd + 1; tStep++) {
        printf("\nstep : %d, time= : %f (s) \n", tStep, time);
        double frame_time = 1.0 / out_fps;
        int next_frame = (int)floor((time + 1e-6) / frame_time) + 1;
        double next_frame_time = next_frame * frame_time;
        double max_allowable_dt = next_frame_time - time;
        if (max_allowable_dt > 1e-6)
            sysFSI.SetMaxStepSize(std::min(Global_max_dT, max_allowable_dt));
        else
            sysFSI.SetMaxStepSize(Global_max_dT);

        sysFSI.DoStepDynamics_FSI();

        auto rhoPresMu = sysFSI.GetParticlePosOrProperties();
        auto vel = sysFSI.GetParticleVel();

        std::ofstream output;
        std::string delim = ",";
        output.open((demo_dir + "/Analysis.txt"), std::ios::app);
        if (tStep == 0)
            output << "Time" << delim << "Rho_fluid" << delim << "k_fluid" << std::endl;

        double KE = 0;
        double Rho = 0;
        for (int i = 0; i < numPart; i++) {
            KE += 0.5 * vel[i].Length();
            Rho += rhoPresMu[i].x();
        }

        output << time << delim << Rho / numPart << delim << sysFSI.GetParticleMass() * KE / numPart << std::endl;
        output.close();
        time += dT;
        SaveParaViewFilesMBD(sysFSI, sysMBS, next_frame, time);
        if (time > t_end)
            break;
    }

    // Total computational cost
    double TIMING_end = (clock() - TIMING_sta) / (double)CLOCKS_PER_SEC;
    printf("\nSimulation Finished in %f (s)\n", TIMING_end);
    
    return 0;
}
