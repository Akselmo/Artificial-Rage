package entity

import rl "vendor:raylib"
import "src:game/settings"

WeaponRifle : WeaponData = {
    name                 = "Rifle",
    inputKey             = settings.keyWeaponThree,
    weaponId             = WeaponID.RIFLE,
    damage               = 3,
    ammo                 = 60, // TODO: Set to 0 for release!
    fireRate             = 0.3,
    range                = 20.0,
    pickedUp             = false,
    maxAmmo              = WEAPON_RIFLE_AMMO_MAX,
    hitscan              = false,
    spriteSpeed          = 10,
    spriteFireFrame      = 2,
    projectileSize       = rl.Vector3{ 0.05, 0.05, 0.1 },
    projectileColor      = rl.BLUE,
    spritesTotal         = 5,
    spritePositionOffset = rl.Vector2{ -0.10, 1.0 }
}
