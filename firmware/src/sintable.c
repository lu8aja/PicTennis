#include "sintable.h" 

#pragma udata

rom const float sintable[] = {
    0,
    0.024930691738072875,
    0.04984588566069716,
    0.07473009358642425,
    0.09956784659581666,
    0.12434370464748518,
    0.14904226617617444,
    0.17364817766693036,
    0.19814614319939758,
    0.2225209339563144,
    0.24675739769029365,
    0.2708404681430051,
    0.2947551744109042,
    0.31848665025168443,
    0.34202014332566877,
    0.36534102436639504,
    0.38843479627469474,
    0.41128710313061156,
    0.4338837391175581,
    0.456210657353163,
    0.47825397862131824,
    0.5,
    0.5214352033794981,
    0.5425462638657594,
    0.5633200580636221,
    0.5837436722347898,
    0.6038044103254774,
    0.6234898018587335,
    0.6427876096865394,
    0.6616858375968595,
    0.6801727377709195,
    0.6982368180860729,
    0.7158668492597184,
    0.7330518718298263,
    0.7497812029677342,
    0.766044443118978,
    0.7818314824680298,
    0.7971325072229225,
    0.8119380057158565,
    0.8262387743159949,
    0.8400259231507714,
    0.8532908816321556,
    0.8660254037844387,
    0.8782215733702285,
    0.8898718088114687,
    0.9009688679024191,
    0.9115058523116732,
    0.9214762118704076,
    0.9308737486442042,
    0.9396926207859084,
    0.9479273461671318,
    0.9555728057861408,
    0.962624246950012,
    0.969077286229078,
    0.9749279121818236,
    0.9801724878485438,
    0.9848077530122081,
    0.9888308262251285,
    0.9922392066001721,
    0.9950307753654014,
    0.9972037971811801,
    0.9987569212189223,
    0.9996891820008162,
    1
};

float simplesin(unsigned char a){
    if (a < 64){
        return  sintable[a];
    }
    if (a < 128){
        return  sintable[127 - a];
    }
    if (a < 192){
        return  sintable[a - 128] * -1;
    }
    return  sintable[255 - a] * -1;
}

float simplecos(unsigned char a){
    if (a < 64){
        return  sintable[63 - a];
    }
    if (a < 128){
        return  sintable[a - 64] * -1;
    }
    if (a < 192){
        return  sintable[191 - a] * -1;
    }
    return  sintable[a - 192];
}