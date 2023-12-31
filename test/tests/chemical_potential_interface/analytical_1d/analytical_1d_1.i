[Mesh]
  [generated]
    type = GeneratedMeshGenerator
    dim = 1
    nx = 100
    xmax = 1
    elem_type = EDGE3    
  []

  # assign two subdomains
  [block1]
    type = SubdomainBoundingBoxGenerator
    input = generated
    block_id = 1
    bottom_left = '0 0 0'
    top_right = '0.6 0 0'
  []
  [block2]
    type = SubdomainBoundingBoxGenerator
    input = block1
    block_id = 2
    bottom_left = '0.6 0 0'
    top_right = '1 0 0'
  []
  [interface]
    type = SideSetsBetweenSubdomainsGenerator
    input = block2
    primary_block = '1'
    paired_block = '2'
    new_boundary = 'interface'
  []
  [break_boundary]
    input = interface
    type = BreakBoundaryOnSubdomainGenerator
  []
[]

[Variables]
  [Mobile]
   block = 1
   order = SECOND
   []
  [m2]
    block = 2
    order = SECOND
  []
[]

[Kernels]
#  [time]
#    type = ADTimeDerivative
#    variable=Mobile
#  []
  [diffusion]
    type = ADMatDiffusion
    variable = Mobile
    diffusivity = D
  []
    [diffusion2]
    type = ADMatDiffusion
    variable = m2
    diffusivity = D2
  []
  [source1]
    type = ADBodyForce
    variable = Mobile
    value = 1.414
  []
  [source2]
    type = ADBodyForce
    variable = m2
    value = 1.414
  []

[]

[InterfaceKernels]
  [chemical]
    type = ADChemicalPotentialInterface
    variable = Mobile
    neighbor_var = m2
    boundary = interface
    s = S
    s_neighbor = S2
    rho = 1
    rho_neighbor = 1
  []
  [diffusion]
    type = ADMatInterfaceDiffusion
    variable = Mobile
    neighbor_var = m2
    boundary = interface
    D = 'D'
    D_neighbor = 'D2'
    rho = 1
    rho_neighbor = 1
  []
[]

[BCs]
  [left]
    type = ADDirichletBC
    variable = Mobile
    value = 1
    boundary = 'left'
  []
  [right]
    type = ADDirichletBC
    variable = m2
    value = 2
    boundary = 'right'
  []
[]

[Materials]
  [left]
    type = ADGenericConstantMaterial
    prop_names = 'D S'
    prop_values = '1 2'
    block = 1
  []
  [right]
    type = ADGenericConstantMaterial
    prop_names = 'D2 S2'
    prop_values = '2 3'
    block = 2
  []
[]

[Executioner]
  type = Steady
  petsc_options_iname = '-ksp_type -pc_type -pc_factor_shift_type'
  petsc_options_value = 'bcgs lu NONZERO'
  nl_rel_tol = 1e-13
[]

[Outputs]
  exodus = true
[]
