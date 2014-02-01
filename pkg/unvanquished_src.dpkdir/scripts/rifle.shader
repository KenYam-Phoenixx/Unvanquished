gfx/rifle/verysmallrock
{
	cull disable
	{
		map gfx/rifle/verysmallrock
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		rgbGen vertex
	}
}

models/weapons/rifle/flash
{
	sort additive
	cull disable
	{
		map	models/weapons/rifle/flash
		tcMod rotate 3000
		blendfunc GL_ONE GL_ONE
	}
}

models/weapons/rifle/zrifle
{
	diffuseMap models/weapons/rifle/zrifle_COLOR
	normalMap models/weapons/rifle/zrifle_NRM
	specularMap models/weapons/rifle/zrifle_SPEC
}

models/weapons/rifle/zriflelens
{
	{
		map models/weapons/rifle/lense
		blendfunc add
		tcGen environment
	}
}

models/weapons/rifle/zsight
{
	diffuseMap models/weapons/rifle/zsight_COLOR
	normalMap models/weapons/rifle/zsight_NRM
	specularMap models/weapons/rifle/zsight_SPEC
}


MarkRifleBullet
{
	polygonOffset
	{
		map gfx/marks/mark_rifle
		//map models/weapons/rifle/lense
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}
