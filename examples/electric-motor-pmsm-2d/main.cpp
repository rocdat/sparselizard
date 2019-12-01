// This code shows how to perfom a magnetostatic analysis on a 2D cross-section of a
// rotating PMSM (permanent magnet synchronous) electric motor. The motor torque is 
// calculated based on the virtual work principle for an increasing mechanical angle.
//
// Anti-periodicity is used to reduce the computational domain to only 45 degrees of 
// the total geometry (the motor has 4 pairs of poles). In order to link the rotor and
// stator domains at their interface a general Mortar-based continuity condition is used.
// This allows to work with the non-matching mesh at the interface when the rotor moves.  


#include "sparselizardbase.h"


using namespace mathop;

// Input is rotor angular position in degrees. Output is torque in Nm.

double sparselizard(double alpha)
{	
    // Give names to the physical region numbers :
    int rotmagmat = 1, magnet = 2, magnetgap = 3, gaprot = 4, gapstat = 5, statmagmat = 6, windslot = 7, winda = 8, windb = 9, windc = 10;
    int gammarot = 11, gammastat = 12, gamma1rot = 13, gamma2rot = 14, gamma1stat = 15, gamma2stat = 16, inarc = 17, outarc = 18;

    // Load the rotor and stator mesh without merging the interface. Set verbosity to 0.
    mesh mymesh(false, {"rotor.msh", "stator.msh"}, 0);

    // Define new physical regions for convenience:
    int rotor = regionunion({rotmagmat, magnet, magnetgap, gaprot});
    int windings = regionunion({winda, windb, windc});
    int stator = regionunion({gapstat, windslot, statmagmat, windings});
    int nonmag = regionunion({magnet, magnetgap, gaprot, gapstat, windslot, windings});
    int gamma1 = regionunion({gamma1rot,gamma1stat});
    int gamma2 = regionunion({gamma2rot,gamma2stat});
    int all = regionunion({rotor,stator});
    
    mymesh.rotate(rotor, 0,0,alpha);

    // Peak winding current [A] times number of turns:
    double Imax = 300;
    // Calculate the area of a winding:
    double windarea = expression(1).integrate(winda, 5);


    // Nodal shape functions 'h1' for the z component of the vector potential.
    field az("h1"), x("x"), y("y"), z("z");

    // In 2D the vector potential only has a z component:
    expression a = array3x1(0,0,az);

    // Use interpolation order 2:
    az.setorder(all, 2);

    // Put a magnetic wall at the inner rotor and outer stator boundaries:
    az.setconstraint(inarc);
    az.setconstraint(outarc);

    // The remanent induction field in the magnet is 0.5 Tesla perpendicular to the magnet:
    expression normedradialdirection = array3x1(x,y,0)/sqrt(x*x+y*y);
    expression bremanent = 0.5 * normedradialdirection;     


    // Vacuum magnetic permeability [H/m]: 
    double mu0 = 4.0*getpi()*1e-7;

    // Define the permeability in all regions.
    //
    // Taking into account saturation and measured B-H curves can be easily done
    // by defining an expression based on a 'spline' object (see documentation).
    //
    parameter mu;

    mu|all = 2000.0*mu0;
    // Overwrite on non-magnetic regions:
    mu|nonmag = mu0;


    formulation magnetostatics;

    // The strong form of the magnetostatic formulation is curl( 1/mu * curl(a) ) = j, with b = curl(a):
    magnetostatics += integral(all, 1/mu* curl(dof(a)) * curl(tf(a)) );

    // Add the remanent magnetization of the rotor magnet:
    magnetostatics += integral(magnet, -1/mu* bremanent * curl(tf(a)));

    // Add the current density source js [A/m2] in the z direction in the stator windings.
    // A three-phased actuation is used. The currents are dephased by the mechanical angle
    // times the number of pole pairs. This gives a stator field rotating at synchronous speed.

    // Change the phase (degrees) to tune the electric angle: 
    double phase = 0;

    parameter jsz;

    jsz|winda = Imax/windarea * sin( (phase + 4.0*alpha - 0.0) * getpi()/180.0);
    jsz|windb = Imax/windarea * sin( (phase + 4.0*alpha - 60.0) * getpi()/180.0);
    jsz|windc = Imax/windarea * sin( (phase + 4.0*alpha - 120.0) * getpi()/180.0);

    magnetostatics += integral(windings, array3x1(0,0,jsz) * tf(a));

    // Rotor-stator continuity condition (including antiperiodicity settings with factor '-1'):
    magnetostatics += continuitycondition(gammastat, gammarot, az, az, {0,0,0}, alpha, 45.0, -1.0);
    // Rotor and stator antiperiodicity condition:
    magnetostatics += periodicitycondition(gamma1, gamma2, az, {0,0,0}, {0,0,45.0}, -1.0);


    solve(magnetostatics);

    az.write(all, "a"+std::to_string((int)alpha)+".vtu", 2);
    curl(a).write(all, "b"+std::to_string((int)alpha)+".vtu", 2);


    // The MAGNETOSTATIC FORCE acting on the rotor is computed below.

    // This field will hold the x and y component of the magnetic forces:
    field magforce("h1xy");

    // The magnetic force is projected on field 'magforce' on the solid rotor region.
    // This is done with a formulation of the type dof*tf - force calculation = 0.
    formulation forceprojection;

    forceprojection += integral(statmagmat, dof(magforce)*tf(magforce));
    expression H = 1/mu * curl(a);
    forceprojection += integral(stator, - predefinedmagnetostaticforce(tf(magforce, statmagmat), H, mu));

    solve(forceprojection);

    // Calculate the torque:
    expression leverarm = array3x1(x,y,0);

    double torque = compz(crossproduct(leverarm, magforce)).integrate(statmagmat, 5);

    // The torque has to be scaled to the actual motor z length (50 mm) and multiplied
    // by 8 to take into account the full 360 degrees of the motor.
    // A minus sign gives the torque on the rotor (opposite sign than the stator torque).
    torque = - torque * 8.0 * 0.05;

    return torque;
}

int main(void)
{	
    SlepcInitialize(0,{},0,0);

    wallclock clk;

    std::cout << "Mechanical angle [degrees] and torque [Nm]:" << std::endl;
    for (double alpha = 0.0; alpha <= 45.0; alpha += 1.0)
    {
        double torque = sparselizard(alpha);
        std::cout << alpha << " " << torque << std::endl;   
    }

    clk.print("Total run time:");

    SlepcFinalize();

    return 0;
}

