#include "enemy.h"
#include "player.h"
#include "projectile.h"
#include "raylib.h"

// Private functions
void Enemy_UpdatePosition(Enemy_Data* enemy);
bool Enemy_TestPlayerHit(Enemy_Data* enemy);
float Enemy_FireAtPlayer(Enemy_Data* enemy, float nextFire);
Ray Enemy_CreateRay(Enemy_Data* enemy);
void Enemy_PlayAnimation(Enemy_Data* enemy, int animationId, float animationSpeed);

// TODO: Rotation
Enemy_Data Enemy_Add(float pos_x, float pos_z, int id)
{
    Vector3 enemyPosition         = (Vector3) { pos_x, ENEMY_POSITION_Y, pos_z };
    Vector3 enemyRotation         = Vector3Zero();
    Vector3 enemySize             = (Vector3) { 0.25f, 0.8f, 0.25f };
    float randomTickRate          = ((float)rand() / (float)(RAND_MAX)) * 2;

    const char modelFileName[128] = "./assets/models/enemy.m3d";
    unsigned int animationsCount  = 0;
    ModelAnimation *animations    =  LoadModelAnimations(modelFileName,&animationsCount);

    Enemy_Model model             = {
        .model = LoadModel(modelFileName),
        .animations = animations,
        .animationsCount = animationsCount,
        .currentAnimation = IDLE,
        .animationPlaying = false
    };

    Enemy_Data enemy = {
        .position      = enemyPosition,
        .rotation      = enemyRotation,
        .size          = enemySize,
        .model         = model,
        .dead          = false,
        .damage        = 5,
        .health        = 15,  // Check enemy health balance later
        .boundingBox   = Utilities_MakeBoundingBox(enemyPosition, enemySize),
        .id            = id,
        .tickRate      = randomTickRate,
        .nextTick      = -1.0f,
        .movementSpeed = ENEMY_DEFAULT_MOVEMENT_SPEED,
        .rotationSpeed = ENEMY_DEFAULT_ROTATION_SPEED,
        .fireRate      = 5.75f,
        .nextFire      = 10.0f,
    };
    return enemy;
}

void Enemy_Update(Enemy_Data* enemy)
{
    Enemy_PlayAnimation(enemy, 3, 1.0f);
    if(!enemy->dead)
    {
        Enemy_Draw(enemy);
        if(enemy->nextTick > 0)
        {
            enemy->nextTick -= GetFrameTime();
        }
        else
        {
            enemy->nextTick = enemy->tickRate;
        }
        Enemy_UpdatePosition(enemy);
        enemy->nextFire -= GetFrameTime();
        enemy->nextFire = Enemy_FireAtPlayer(enemy, enemy->nextFire);
    }
}

void Enemy_Draw(Enemy_Data* enemy)
{
    DrawModel(enemy->model.model, enemy->position, 0.5f, WHITE);
}

Ray Enemy_CreateRay(Enemy_Data* enemy)
{
    Ray rayCast;
    BoundingBox playerBb   = Player->boundingBox;
    Vector3 playerPosition = Player->position;
    Vector3 v              = Vector3Normalize(Vector3Subtract(enemy->position, playerPosition));
    rayCast.direction      = (Vector3) { -1.0f * v.x, -1.0f * v.y, -1.0f * v.z };
    rayCast.position       = enemy->position;
    return rayCast;
}

bool Enemy_TestPlayerHit(Enemy_Data* enemy)
{

    Ray rayCast = Enemy_CreateRay(enemy);

    bool hitPlayer        = false;
    float distance        = 0.0f;
    float levelDistance   = INFINITY;
    float playerDistance  = INFINITY;
    int entitiesAmount    = Level_mapSize;
    Level_Data* levelData = Level_data;
    Level_Data levelDataHit;

    for(int i = 0; i < entitiesAmount; i++)
    {
        if(levelData[i].modelId != 0)
        {
            Vector3 pos           = levelData[i].blockPosition;
            RayCollision hitLevel = GetRayCollisionMesh(
                rayCast, levelData[i].blockModel.meshes[0], MatrixTranslate(pos.x, pos.y, pos.z));
            if(hitLevel.hit)
            {
                if(hitLevel.distance < levelDistance)
                {
                    levelDistance = hitLevel.distance;
                    levelDataHit  = levelData[i];
                }
            }
        }
    }

    playerDistance = Vector3Length(Vector3Subtract(Player->position, rayCast.position));

    if(playerDistance < levelDistance)
    {
        // Player is closer
        hitPlayer = true;
    }
    else
    {
        // Wall/other entity is closer so we didnt hit player
        hitPlayer = false;
    }
    return hitPlayer;
}

void Enemy_UpdatePosition(Enemy_Data* enemy)
{
    // Move enemy towards player
    Vector3 DistanceFromPlayer = Vector3Subtract(enemy->position, Player->position);
    //- Check if player can be seen (first raycast hit returns player)
    if(Enemy_TestPlayerHit(enemy))
    {
        //- If in certain range from player, stop
        if(fabsf(DistanceFromPlayer.x) >= ENEMY_MAX_DISTANCE_FROM_PLAYER ||
           fabsf(DistanceFromPlayer.z) >= ENEMY_MAX_DISTANCE_FROM_PLAYER)
        {
            Vector3 enemyOldPosition = enemy->position;
            Vector3 enemyNewPosition = (Vector3){Player->position.x, ENEMY_POSITION_Y,Player->position.z};
            enemy->position = Vector3Lerp(enemy->position,enemyNewPosition,enemy->movementSpeed * GetFrameTime());
            if(Level_CheckCollision(enemy->position, enemy->size, enemy->id))
            {
                enemy->position = enemyOldPosition;
                return;
            }
        }
    }
    enemy->boundingBox = Utilities_MakeBoundingBox(enemy->position, enemy->size);
}

void Enemy_TakeDamage(Enemy_Data* enemy, int damageAmount)
{
    if(!enemy->dead)
    {
        enemy->health -= damageAmount;
        printf("Enemy id %d took %d damage\n", enemy->id, damageAmount);
        if(enemy->health <= 0)
        {
            // Dirty hack to move bounding box outside of map so it cant be collided to.
            // We want to keep enemy in the memory so we can use its position to display the
            // corpse/death anim
            Vector3 deadBoxPos = (Vector3) { ENEMY_GRAVEYARD_POSITION,
                                             ENEMY_GRAVEYARD_POSITION,
                                             ENEMY_GRAVEYARD_POSITION };
            enemy->boundingBox = Utilities_MakeBoundingBox(deadBoxPos, Vector3Zero());
            enemy->dead        = true;
        }
    }
}

float Enemy_FireAtPlayer(Enemy_Data* enemy, float nextFire)
{
    if(Enemy_TestPlayerHit(enemy))
    {
        Enemy_RotateTowards(enemy, Player->position);
        if(nextFire > 0)
        {
            nextFire -= GetFrameTime();
        }
        else
        {
            // Fire animation should play before we shoot projectile
            // TODO: dont shoot before level is loaded!!

            Projectile_Create(
                Enemy_CreateRay(enemy), (Vector3) { 0.2f, 0.2f, 0.2f }, enemy->damage, enemy->id);
            nextFire = enemy->fireRate;
        }
    }
    return nextFire;
}

void Enemy_RotateTowards(Enemy_Data* enemy, Vector3 targetPosition)
{
    // Rotates the enemy around Y axis
    Vector3 diff        = Vector3Subtract(enemy->position, targetPosition);
    float y_angle       = -(atan2(diff.z, diff.x) + PI / 2.0);
    Vector3 newRotation = (Vector3) { 0, y_angle, 0 };

    Quaternion start = QuaternionFromEuler(enemy->rotation.z, enemy->rotation.y, enemy->rotation.x);
    Quaternion end   = QuaternionFromEuler(newRotation.z, newRotation.y, newRotation.x);
    Quaternion slerp = QuaternionSlerp(start, end, enemy->rotationSpeed * GetFrameTime());

    enemy->model.model.transform = QuaternionToMatrix(slerp);
    enemy->rotation              = newRotation;
}

void Enemy_PlayAnimation(Enemy_Data* enemy, int animationId, float animationSpeed)
{
    const int frameCount = enemy->model.animations[enemy->model.currentAnimation].frameCount;

    if(enemy->model.currentAnimation != animationId && enemy->model.animationFrame >= frameCount)
    {
        enemy->model.currentAnimation = animationId;
    }

    enemy->model.animationFrame   = Utilities_PlayAnimation(enemy->model.model,
                                                          &enemy->model.animations[enemy->model.currentAnimation],
                                                          animationSpeed,
                                                          enemy->model.animationFrame,
                                                          animationId);

    if(enemy->model.animationFrame > frameCount)
    {
        enemy->model.animationFrame = 0;
    }
    //printf("%d / %d\n",enemy->model.animationFrame, frameCount);

}