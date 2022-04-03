// FosterMcNabbTrapAction
// "achlysApp"
#include "Conversion.h"
#include "FEProblem.h"
#include "Factory.h"
#include "MooseMesh.h"
#include "MooseObjectAction.h"
#include "FosterMcNabbTrapAction.h"
#include "Material.h"

#include "BlockRestrictable.h"

// #include "HomogenizationConstraintIntegral.h" // just for the constants
#include "AddVariableAction.h"

#include "libmesh/string_to_enum.h"
#include <algorithm>

// registerMooseAction("achlysApp", FosterMcNabbTrapAction, "meta_action");
// registerMooseAction("achlysApp", FosterMcNabbTrapAction, "setup_mesh_complete");
// registerMooseAction("achlysApp", FosterMcNabbTrapAction, "validate_coordinate_systems");
registerMooseAction("achlysApp", FosterMcNabbTrapAction, "add_variables");
// registerMooseAction("achlysApp", FosterMcNabbTrapAction, "add_aux_variable");
registerMooseAction("achlysApp", FosterMcNabbTrapAction, "add_kernels");
// registerMooseAction("achlysApp", FosterMcNabbTrapAction, "add_aux_kernel");
registerMooseAction("achlysApp", FosterMcNabbTrapAction, "add_materials");
// registerMooseAction("achlysApp", FosterMcNabbTrapAction, "add_master_action_material");
// registerMooseAction("achlysApp", FosterMcNabbTrapAction, "add_scalar_kernel");
// registerMooseAction("achlysApp", FosterMcNabbTrapAction, "add_user_object");

/*
    1) create new trap_i variables for each element given in material property vectors (for each material subdomain)
        i) consider variable order and family

    2) declare new material properties 

    3) create 3 kernels for the Foster-McNabb trapping model, including coupling with the mobile term

    4) instantiate interface kernels if required

    5) create summary aux-variables which summarise global mobile / trapped HI in SI units
        i) option to disable if requested

    Questions:
    i) create mobile variables here too? -- how will the user-object ecosystem look?
    ii) global variable options? (seen some convergence issues just being block-restricted)
    iii) can this generalise to multi-isotope models?
    iv) switch between molar and atomic units? 

    -- create new data strcutures or just use naive loops through input variables to start?

    -- BCs etc. will wlays be in terms of mobile species so maybe best to set these explicitly outsid eof the action?

*/

InputParameters
FosterMcNabbTrapAction::validParams()
{
    InputParameters params = Action::validParams();
    // params += BlockRestrictable::validParams();
    params.addClassDescription("Set up kernels and materials for a Foster-McNabb trapping model");
    // params.addParam<std::vector<MaterialPropertyName>>("material_definitions", "material classes providing input data");
    params.addRequiredParam<std::vector<Real>>("v0", "pre-exponential detrapping factor in Arrhenious eq.");
    params.addRequiredParam<std::vector<Real>>("E", "Trap detrapping energy in eV");
    params.addRequiredParam<std::vector<Real>>("n", "possible trapping sites");
    params.addRequiredParam<Real>("k", "Boltzman constant");
    params.addRequiredParam<Real>("D0", "The diffusion pre-exponential factor");
    params.addRequiredParam<Real>("Ed", "diffusion energy in eV");
    params.addParam<Real>("S0", 0.0, "The solubility pre-exponential factor");
    params.addParam<Real>("Es", 0.0, "energy of solution in eV");
    params.addParam<Real>("p0", -1, "Explicit pre-exponential factor for the trapping reaction rate (bypasses internal calculaution based on D(T) and lambda)");
    params.addParam<Real>("Ep", -1, "Explicit binding energy for the trapping reaction rate");
    params.addParam<Real>("lambda", -1, "Lattice constant in m-1");
    params.addParam<Real>("n_sol", -1, "density of interstitial sites in atomic fraction");
    params.addParam<Real>("atomic_density", 1.0, "Number density of solute atoms in m^-3");
    params.addRequiredParam<std::string>("Temperature","simulation temperature");
    params.addParam<std::string>("trap_variable_base", "trap", "the base name for automatically created trap variables");
    params.addParam<std::string>("mobile_variable_base", "mobile", "the base name for automatically created mobile variables");
    params.addParam<std::string>("detrap_material_base", "detrapping_rate", "the base name for the de-trapping rate material property");
    params.addParam<std::string>("trapping_material_base", "trapping_rate", "the base name for the trapping rate material property");
    params.addParam<std::string>("trap_density_material_base", "trap_density", "the base name for trap density material property");
    params.addParam<std::string>("diffusivity_material_base", "D", "the base name for the diffusivity material property");
    params.addParam<std::string>("solubility_material_base", "S", "the base name for the solubility material property");
    params.addParam<std::vector<SubdomainName>>("block", "optional list of subdomain IDs this action applies to");
    // enum for order of variables
    // enum for molar or eV formulation
    // interface type -- global variables or block restricted?
    // handle variable trap densities 
    // varying requirements for input variables e.g. solubility for CP interface but not otherwise
    return params;
}

FosterMcNabbTrapAction::FosterMcNabbTrapAction(const InputParameters & params)
  : Action(params),
//   BlockRestrictable(this),
    // _material_definition_names(getParam<std::vector<MaterialPropertyName>>("material_definitions"))
    _n(getParam<std::vector<Real>>("n")),
    _v0(getParam<std::vector<Real>>("v0")),
    _E(getParam<std::vector<Real>>("E")),
    _temperature_variable(getParam<std::string>("Temperature")),
    _k(getParam<Real>("k")),
    _D0(getParam<Real>("D0")),
    _Ed(getParam<Real>("Ed")),
    _S0(getParam<Real>("S0")),
    _Es(getParam<Real>("Es")),
    _p0(getParam<Real>("p0")),
    _Ep(getParam<Real>("Ep")),
    _lambda(getParam<Real>("lambda")),
    _n_sol(getParam<Real>("n_sol")),
    _rho(getParam<Real>("atomic_density")),
    _trap_variable_base(getParam<std::string>("trap_variable_base")),
    _mobile_variable_base(getParam<std::string>("mobile_variable_base")),
    _detrap_material_base(getParam<std::string>("detrap_material_base")),
    _trap_material_base(getParam<std::string>("trapping_material_base")),
    _trap_density_material_base(getParam<std::string>("trap_density_material_base")),
    _diffusivity_material_base(getParam<std::string>("diffusivity_material_base")),
    _solubility_material_base(getParam<std::string>("solubility_material_base")),
    _blocks(getParam<std::vector<SubdomainName>>("block"))
    

{
    //   determine order of variables to be created 
    //    // verifyOrderAndFamilyOutputs();
    //  use displaced mesh?
    // consistency checks
    
    bool equal_arrays = _v0.size() == _n.size() && _E.size() == _n.size();
    if (! equal_arrays)
    {
        mooseError("The trap parameter vectors v0, ni, and Ei must be the same length");
    }
    else
    {
        _n_traps = _v0.size();
    }

    _trapping_rate_specified = params.isParamSetByUser("p0");
    _trapping_energy_specified = params.isParamSetByUser("Ep");
    if (!_trapping_rate_specified)
    {
       bool required_params_specified = params.isParamSetByUser("lambda") && params.isParamSetByUser("n_sol");
       if (!required_params_specified)
       {
           mooseError("Trapping reaction rate must either be specified using (i) the lattice spacing parameter, lambda, "
            "and the number of solute sites per lattice atom, n_sol, or (ii) explicitly setting the trapping rate pre-exponential "
            "factor, p0.");
       }
    }
    else if (!_trapping_energy_specified)
    {
        _Ep = _Ed;
    }
    _solubility_specified = params.isParamSetByUser("S0") &&params.isParamSetByUser("Es");

    /*
        construct unique lists of variable names
    */
    _block_prepend = "";
    // if (_par.isParamSetByUser("block"))
    if(! _blocks.empty())
    {
        // from block-resitrable class the list of user_suppled SubdominNames is stored here
        _block_prepend = std::string("_") + std::string(_blocks[0]);
    }

    for (int i = 0; i < _n_traps; i++)
    {
        std::string name = _trap_variable_base + _block_prepend + "_" + std::to_string(i +1);
        _trap_variable_names.push_back(name);

        std::string trap_densityty_name = _trap_density_material_base + _block_prepend + "_" + std::to_string(i +1);
        _trap_density_names.push_back(trap_densityty_name);

        std::string detrappign_rate_name = _detrap_material_base + _block_prepend + "_" + std::to_string(i +1);
        _detrapping_rate_names.push_back(detrappign_rate_name);
    }
    _mobile_variable_name = _mobile_variable_base + _block_prepend;
    _all_variable_names =  _trap_variable_names;
    _all_variable_names.push_back(_mobile_variable_name);

    _trapping_rate_material_name = _trap_material_base + _block_prepend;
    _diffusivity_material_name = _diffusivity_material_base + _block_prepend;
    _solubility_material_name = _solubility_material_base + _block_prepend;

    // _transient = _problem->isTransient();
}


void
FosterMcNabbTrapAction::act()
{
    if (_current_task == "add_variables")
    {
        addVariables();
    }
    if (_current_task == "add_materials")
    {
        addMaterials();
    }
    if (_current_task == "add_kernels")
    {
        addTrappingReactionKernels();

        if (_problem->isTransient())
        {
            addTimeKernels();
            addTrapCouplingKernels();
        }

        addDiffusionKernel();
    }
}


void FosterMcNabbTrapAction::addVariables()
{
    auto params = _factory.getValidParams("MooseVariable");
    // const bool second = _order_specified ? _order == "SECOND" : _problem->mesh().hasSecondOrderElements();
    const bool second_order =  _problem->mesh().hasSecondOrderElements();
    params.set<MooseEnum>("order") = second_order ? "SECOND" : "FIRST";
    params.set<MooseEnum>("family") = "LAGRANGE";
    if (!_blocks.empty())
    {
        params.set<std::vector<SubdomainName>>("block") = _blocks; 
    }

    for (const auto & name : _all_variable_names)
    {
        _problem->addVariable("MooseVariable", name, params);
    }
}

void FosterMcNabbTrapAction::addMaterials()
{
    // addArrheniousMaterials(_traps); // could still use vector material here possibly...
    addArrheniusMaterial("D", _D0, _Ed);
    if (_solubility_specified) addArrheniusMaterial("S", _S0, _Es);

    if (_trapping_rate_specified)
    {
        addArrheniusMaterial(_trapping_rate_material_name, _p0, _Ep);
    }
    else
    {
        Real p0 = _D0 / ( std::pow(_lambda, 2) * _n_sol);
        addArrheniusMaterial(_trapping_rate_material_name, p0, _Ed);
    }

    addDetrappingRateMaterials();
    addGenericConstantMaterial(_trap_density_names, _n);
    addGenericConstantMaterial({"rho"}, {_rho});
    
}

// void FosterMcNabbTrapAction::addArrheniusMaterials(std::map<std::string, FosterMcNabbTrapAction::trap_parameters> & names)
// {
//     /*
//         1) block restrictions? 
//     */
//     std::string type = "ArrheniusMaterial"
//     for (auto const& [name, values] : names) // c++17
//     {
//         params = _factory.getValidParams(type);
//         params.set<MaterialPropertyName>("name") = name;
//         params.set<Real>("v0") = values.v0; // if we make a struct can call by name to be more explicit
//         params.set<Real>("E") = values.E;
//         params.set<Real>("k") = _k; //assume we don't want to mix eV and kJ/mol anyway so let's keep this constant
//         params.set<NonlinearVariableName>("Temperature") = _temperature_variable;
//         std::string material_block_name = name + "_material"
//         _problem->addMaterial(type, material_block_name, params);
        
//     }
// }

void FosterMcNabbTrapAction::addDetrappingRateMaterials()
{
    std::string type = "ArrheniusMaterial";
    for (int i = 0; i< _n_traps; i++)
    {
        auto params = _factory.getValidParams(type);
        std::string material_property_name = _detrapping_rate_names[i]; 
        //_detrap_material_base + "_" + std::to_string(i + 1);
        // params.set<MaterialPropertyName>("name") = material_property_name;
        params.set<std::string>("name") = material_property_name;
        params.set<Real>("v0") = _v0[i];
        params.set<Real>("E") = _E[i];
        params.set<Real>("k") = _k;
        // params.set<NonlinearVariableName>("Temperature") = _temperature_variable;
        params.set<std::vector<VariableName>>("Temperature") = {_temperature_variable};
        std::string material_block_name = material_property_name + "_material" + _block_prepend;
        if (!_blocks.empty())
        {
            params.set<std::vector<SubdomainName>>("block") = _blocks; 
        }
        params.set<std::vector<OutputName>>("outputs") = {"exodus"}; 
        _problem->addMaterial(type, material_block_name, params);
    }
}

// void FosterMcNabbTrapAction::addDetrappingRateMaterials()
// {
    
// }

// void FosterMcNabbTrapAction::addArrheniusMaterials(std::map<std::string, std::pair<Real, Real> & names)
// {

//     std::string type = "ArrheniusMaterial"
//     for (auto const& [name, values] : names) // c++17
//     {
//         params = _factory.getValidParams(type);
//         params.set<MaterialPropertyName>("name") = name;
//         params.set<Real>("v0") = values.first; // if we make a struct can call by name to be more explicit
//         params.set<Real>("E") = values.second;
//         params.set<Real>("k") = _k; //assume we don't want to mix eV and kJ/mol anyway so let's keep this constant
//         params.set<NonlinearVariableName>("Temperature") = _temperature_variable;
//         std::string material_block_name = name + "_material";
//         // _problem->addMaterial(type, material_block_name, params);
//         if (!_blocks.empty())
//         {
//             params.set<std::vector<SubdomainName>>("blocks") = _blocks; 
//         }
//          _problem->addMaterial(type, material_block_name, params);
        
//     }
// }


void FosterMcNabbTrapAction::addArrheniusMaterial(std::string name, Real V0, Real E)
{
    std::string type = "ArrheniusMaterial";
    auto params = _factory.getValidParams(type);
    if (!_blocks.empty())
    {
        params.set<std::vector<SubdomainName>>("block") = _blocks; 
    }
    // params.set<MaterialPropertyName>("name") = name;
    params.set<std::string>("name") = name;
    params.set<Real>("v0") = V0;
    params.set<Real>("E") = E;
    params.set<Real>("k") = _k; 
    params.set<std::vector<VariableName>>("Temperature") = {_temperature_variable};
    // params.set<std::string>("Temperature") = _temperature_variable;
    params.set<std::vector<OutputName>>("outputs") = {"exodus"}; 
    std::string material_block_name = name + "_material" + _block_prepend;
    // if (!_blocks.empty())
    // {
    //     params.set<std::vector<SubdomainName>>("block") = _blocks; 
    // }
    _problem->addMaterial(type, material_block_name, params);

}

void FosterMcNabbTrapAction::addGenericConstantMaterial(std::vector<std::string> names, std::vector<Real> values)
{
    std::string type = "ADGenericConstantMaterial";
    auto params = _factory.getValidParams(type);
    params.set<std::vector<std::string>>("prop_names") = names;
    params.set<std::vector<Real>>("prop_values") = values;
    std::string material_block_name = names[0] + "_material" + _block_prepend;
    if (!_blocks.empty())
    {
        params.set<std::vector<SubdomainName>>("block") = _blocks; 
    }
    params.set<std::vector<OutputName>>("outputs") = {"exodus"}; 
    _problem->addMaterial(type, material_block_name, params);
}

// void FosterMcNabbTrapAction::addTrappingKernels(std::vec)
// {

// }

// void FosterMcNabbTrapAction::addTrappingReactionKernels(std::vector<std::pair<std::string,std::string>> mobile_trap_pairs)
// {
//     std::string kernel_type = "ADTrappingEquilibriumEquation";
//     for (auto variable_pair : mobile_trap_pairs)
//   {
//     std::string trap_variable_name = variable_pair.first;
//     std::string mobile_variable_name = variable_pair.second;

//     InputParameters params = _factory.getValidParams(kernel_type);
//     params.set<NonlinearVariableName>("v") = mobile_variable_name;
//     params.set<NonlinearVariableName>("variable") = trap_variable_name;
//     std::string kernel_name = trap_variable_name + "_trapping_reaction"
//     params.set<MaterialPropertyName>("vi") = _detrapping_rates[trap_variable_name];
//     params.set<MaterialPropertyName>("n_traps") = _trap_densities[trap_variable_name];
//     _problem->addKernel(kernel_type, kernel_name, params);
//   }
// }
void FosterMcNabbTrapAction::addTrappingReactionKernels()
{
    std::string kernel_type = "ADTrappingEquilibriumEquation";
    for (int i = 0; i < _n_traps; i++)
    {
        InputParameters params = _factory.getValidParams(kernel_type);
        params.set<std::vector<VariableName>>("v") = {_mobile_variable_name};
        params.set<NonlinearVariableName>("variable") = _trap_variable_names[i];
        std::string kernel_name = _trap_variable_names[i] + "_trapping_reaction";
        params.set<MaterialPropertyName>("vi") = _detrapping_rate_names[i];
        params.set<MaterialPropertyName>("n_traps") = _trap_density_names[i];
        params.set<MaterialPropertyName>("Vm") = {_trapping_rate_material_name};
        // if (!_blocks.empty())
        // {
        //     params.set<std::vector<SubdomainName>>("blocks") = _blocks; 
        // }
        _problem->addKernel(kernel_type, kernel_name, params);
    }
}

// void FosterMcNabbTrapAction::addTimeKernels(std::vector<std::string>> variables)
// {
//   std::string kernel_type = "ADTimeDerivative";
//   for (auto variable_name : variables)
//   {
//     InputParameters params = _factory.getValidParams(kernel_type);
//     params.set<NonlinearVariableName>("variable") = variable_name;
//     std::string kernel_name = variable_name + "_dt"
//     _problem->addKernel(kernel_type, kernel_name, params);
//   }
// }

void FosterMcNabbTrapAction::addTimeKernels()
{
  std::string kernel_type = "ADTimeDerivative";
  for (auto variable_name : _all_variable_names)
  {
    InputParameters params = _factory.getValidParams(kernel_type);
    params.set<NonlinearVariableName>("variable") = variable_name;
    std::string kernel_name = variable_name + "_dt";    
    _problem->addKernel(kernel_type, kernel_name, params);
  }
}

void FosterMcNabbTrapAction::addTrapCouplingKernels()
{
    std::string kernel_type = "ADCoupledTimeDerivative";
  for (int i = 0; i < _n_traps; i++)
    {
        InputParameters params = _factory.getValidParams(kernel_type);
        params.set<std::vector<VariableName>>("v") = {_trap_variable_names[i]};
        params.set<NonlinearVariableName>("variable") = _mobile_variable_name;
        std::string kernel_name = _trap_variable_names[i] + "_coupled_dt";
        _problem->addKernel(kernel_type, kernel_name, params);
    }
}

void FosterMcNabbTrapAction::addDiffusionKernel()
{
    std::string kernel_type = "ADMatDiffusion";
    InputParameters params = _factory.getValidParams(kernel_type);
    params.set<NonlinearVariableName>("variable") = _mobile_variable_name;
    params.set<MaterialPropertyName>("diffusivity") = "D";
    std::string kernel_name = _mobile_variable_name + "diffusion";
    _problem->addKernel(kernel_type, kernel_name, params);
}

// // maybe better to have vector< pair< string,vector<string> > > 
// // as we expect multiple traps to be associated with each mobile
// void FosterMcNabbTrapAction::addTrapCouplingKernels(std::vector<std::pair<std::string,std::string>> mobile_trap_pairs)
// {
//   std::string kernel_type = "ADCoupledTimeDerivative";
//   for (auto variable_pair : mobile_trap_pairs)
//   {
//     std::string trap_variable_name = variable_pair.first;
//     std::string mobile_variable_name = variable_pair.second;

//     InputParameters params = _factory.getValidParams(kernel_type);
//     params.set<NonlinearVariableName>("v") = trap_variable_name;
//     params.set<NonlinearVariableName>("variable") = mobile_variable_name;
//     std::string kernel_name = trap_variable_name + "_coupled_dt"
//     _problem->addKernel(kernel_type, kernel_name, params);

//   }
// }
