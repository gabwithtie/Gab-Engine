#include "AnimoBuilder.h"

gbe::ext::AnimoBuilder::AnimoBuilder::AnimoBuilder()
{
}

gbe::ext::AnimoBuilder::AnimoBuilder::~AnimoBuilder()
{
}

gbe::ext::AnimoBuilder::GenerationResult gbe::ext::AnimoBuilder::AnimoBuilder::Generate(GenerationParams params)
{
	GenerationResult res{};

	//PCG FUNCTION

	//STEP 1: CALCULATE VECTORS
	Vector3 delta = params.to - params.from;
	Vector3 stepdir = delta.Normalize();
	Vector3 forward = stepdir.Cross(params.up).Normalize();
	float abdistance = delta.Magnitude();
	float halfheight = params.height * 0.5f;

	auto add = [&](std::string type, Vector3 position, Vector3 scale) {
		res.meshes.push_back({
			.type = type,
			.position = position,
			.scale = scale,
			.rotation = Quaternion::LookAtRotation(forward, params.up)
			});
		};

	//STEP 2: EXECUTE MAIN WALL SEGMENT LOOP
	for (float x = 0; x < abdistance; x += params.pillarInterval)
	{
		float final_width = params.pillarInterval;

		if (x + (params.pillarInterval * 2.0f) > abdistance) {
			final_width = abdistance - x;
		}

		//STEP 2.1: PILLAR PLACEMENT
		Vector3 pillarpos = stepdir * x;
		pillarpos += params.up * halfheight;
		pillarpos += forward * (params.wallThickness * 0.5f);
		Vector3 pillarscale = Vector3(0.5f) * params.pillarThickness;
		pillarscale.y = halfheight;

		add("pillar", params.from + pillarpos, pillarscale);

		//STEP 2.2.1: WALL SEGMENT PLACEMENT
		Vector3 wallpos = stepdir * (x + (final_width * 0.5f));
		wallpos += params.up * halfheight;
		Vector3 wallscale = Vector3(final_width * 0.5f, halfheight, params.wallThickness * 0.5f);

		add("wall", params.from + wallpos, wallscale);

		//STEP 2.2.2: BASE SEGMENT PLACEMENT
		Vector3 basepos = stepdir * (x + (final_width * 0.5f));
		basepos -= params.up * (params.base_height * 0.5f);
		Vector3 basescale = Vector3(final_width * 0.5f, (params.base_height * 0.5f), params.base_width * 0.5f);

		add("base", params.from + basepos, basescale);

		//STEP 2.3 BEAM PLACEMENT
		for (float y = params.beamInterval; y < params.height; y += params.beamInterval)
		{
			Vector3 beampos = stepdir * (x + (final_width * 0.5f));
			beampos += params.up * y;
			Vector3 beamscale = Vector3(final_width * 0.5f, params.beamThickness * 0.5f, params.beamThickness * 0.5f);

			add("beam", params.from + beampos, beamscale);
		}

		//STEP 2.4 WINDOW PLACEMENT
		for (float y = 0; y + params.beamInterval < params.height; y += params.beamInterval)
		{
			Vector3 windowpos = stepdir * (x + (final_width * 0.5f));
			windowpos += params.up * (y + params.windowHeight + (params.windowSize.y * 0.5f));
			windowpos += forward * ((params.wallThickness * 0.5f) + (params.windowSize.z * 0.5f));
			Vector3 windowscale = params.windowSize * 0.5f;

			add("window", params.from + windowpos, windowscale);
		}

		//STEP 2.5 ROOF PLACEMENT
		Vector3 roofpos = stepdir * (x + (final_width * 0.5f));
		roofpos += params.up * (params.height + (params.roofHeight * 0.5f));
		Vector3 roofscale = Vector3(final_width * 0.5f, params.roofHeight * 0.5f, params.roofThickness * 0.5f);

		add("roof", params.from + roofpos, roofscale);
	}

	//STEP 3 END PILLAR PLACEMENT
	Vector3 pillarpos = stepdir * abdistance;
	pillarpos += params.up * halfheight;
	pillarpos += forward * (params.wallThickness * 0.5f);
	Vector3 pillarscale = Vector3(0.5f) * params.pillarThickness;
	pillarscale.y = halfheight;

	add("pillar", params.from + pillarpos, pillarscale);

	return res;
}
