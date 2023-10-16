#include "entity.h"

// Entities have shared functions
bool Entity_UpdatePosition(Entity *entity);
bool Entity_TestPlayerHit(Entity *entity);
bool Entity_FireAtPlayer(Entity *entity, float nextFire);
Ray Entity_CreateRay(Entity *entity);

// Entities used in the game
// This list is used only for checking the entities from
// the map data. If map data matches the same id as the
// position of the template in the EntityList array
// scene will then create a new entity matching the template
struct EntityTemplate *EntityTemplate_list[ENTITIES_TOTAL];
struct EntityTemplate EntityTemplate_none  = { .type = SCENE_NONE, .textureFileName = "", .modelFileName = "" };
struct EntityTemplate EntityTemplate_start = { .type = SCENE_START, .textureFileName = "", .modelFileName = "" };
struct EntityTemplate EntityTemplate_end   = { .type = SCENE_END, .textureFileName = "", .modelFileName = "" };
struct EntityTemplate EntityTemplate_wall1 = { .type            = SCENE_WALL,
	                                           .textureFileName = "./assets/textures/wall1.png",
	                                           .modelFileName   = "" };
struct EntityTemplate EntityTemplate_wall2 = { .type            = SCENE_WALL,
	                                           .textureFileName = "./assets/textures/wall2.png",
	                                           .modelFileName   = "" };
struct EntityTemplate EntityTemplate_enemy = { .type            = SCENE_ACTOR,
	                                           .textureFileName = "",
	                                           .modelFileName   = "./assets/models/enemy.m3d" };
struct EntityTemplate EntityTemplate_item  = { .type = SCENE_ITEM, .textureFileName = "", .modelFileName = "" };

void Entity_InitList(void)
{
	// In Tiled, Entity_none is not used.
	EntityTemplate_list[0] = &EntityTemplate_none;
	// Thus, in Tiled, Entity_start id is 0.
	// NOTE: Make sure the entity id's are in sync with the position they have in this array.
	// Example: Entity_list[TiledId+1] = &Entity_at_that_id;
	EntityTemplate_list[1] = &EntityTemplate_start;
	EntityTemplate_list[2] = &EntityTemplate_end;
	EntityTemplate_list[3] = &EntityTemplate_wall1;
	EntityTemplate_list[4] = &EntityTemplate_wall2;
	EntityTemplate_list[5] = &EntityTemplate_enemy;
	EntityTemplate_list[6] = &EntityTemplate_item;
	// TODO: Likely have to do it's own entity per item. This is just for getting started
}

void Entity_Update(Entity *entity)
{
	Entity_Draw(entity);
	if (entity->type != SCENE_ACTOR)
	{
		return;
	}

	if (!entity->actor.dead)
	{
		if (Entity_TestPlayerHit(entity))
		{
			entity->actor.playerSpotted = true;
			if (Entity_FireAtPlayer(entity, entity->actor.nextFire))
			{
				// TODO: instead of directly changing the animation, use an animator that handles
				//       the animation loops
				Animator_SetAnimation(&entity->actor.animator, ATTACK);
			}
			else
			{
				if (Entity_UpdatePosition(entity))
				{
					Animator_SetAnimation(&entity->actor.animator, MOVE);
				}
				else
				{
					Animator_SetAnimation(&entity->actor.animator, IDLE);
				}
			}
			entity->actor.nextFire -= GetFrameTime();
		}
	}
	else
	{
		Animator_SetAnimation(&entity->actor.animator, DEATH);
	}
	entity->actor.animator.nextFrame -= GetFrameTime();
	entity->actor.animator.nextFrame = Animator_PlayAnimation(
		&entity->actor.animator, &entity->model, ACTOR_DEFAULT_ANIMATION_SPEED, entity->actor.animator.nextFrame
	);
}

void Entity_Draw(Entity *entity) { DrawModel(entity->model, entity->position, entity->scale, WHITE); }

Ray Entity_CreateRay(Entity *entity)
{

	const Vector3 playerPosition = Player->position;
	const Vector3 v              = Vector3Normalize(Vector3Subtract(entity->position, playerPosition));

	Ray rayCast = {
		.direction = (Vector3){-1.0f * v.x, -1.0f * v.y, -1.0f * v.z},
          .position = entity->position
	};

	return rayCast;
}

bool Entity_TestPlayerHit(Entity *entity)
{
	// TODO: this function can be quite heavy, could give it a tickrate?
	//  every 1-2 seconds instead of every frame
	if (entity->type != SCENE_ACTOR)
	{
		return false;
	}

	if (Vector3Distance(Player->position, entity->position) > 5.0f && !entity->actor.playerSpotted)
	{
		return false;
	}
	const Ray rayCast = Entity_CreateRay(entity);

	bool hitPlayer           = false;
	float levelDistance      = INFINITY;
	float playerDistance     = INFINITY;
	const int entitiesAmount = scene->size;
	const Entity *entities   = scene->entities;

	for (int i = 0; i < entitiesAmount; i++)
	{
		if (entities[i].id != 0 && entities[i].id != entity->id)
		{
			Vector3 pos = entities[i].position;
			RayCollision hitLevel =
				GetRayCollisionMesh(rayCast, entities[i].model.meshes[0], MatrixTranslate(pos.x, pos.y, pos.z));
			if (hitLevel.hit)
			{
				if (hitLevel.distance < levelDistance)
				{
					levelDistance = hitLevel.distance;
				}
			}
		}
	}

	playerDistance = Vector3Length(Vector3Subtract(Player->position, rayCast.position));

	// Player is closer
	hitPlayer = (playerDistance < levelDistance);

	return hitPlayer;
}

// Make this boolean: moving or not
bool Entity_UpdatePosition(Entity *entity)
{
	if (entity->type != SCENE_ACTOR)
	{
		return false;
	}

	bool moving = true;
	// Move entity towards player
	const Vector3 DistanceFromPlayer = Vector3Subtract(entity->position, Player->position);
	//- Check if player can be seen (first raycast hit returns player)

	//- If in certain range from player, stop
	if (fabsf(DistanceFromPlayer.x) >= ACTOR_MAX_DISTANCE_FROM_PLAYER ||
	    fabsf(DistanceFromPlayer.z) >= ACTOR_MAX_DISTANCE_FROM_PLAYER)
	{
		const Vector3 entityOldPosition = entity->position;
		const Vector3 entityNewPosition = (Vector3){ Player->position.x, ACTOR_POSITION_Y, Player->position.z };
		entity->position =
			Vector3Lerp(entity->position, entityNewPosition, entity->actor.movementSpeed * GetFrameTime());
		if (Scene_CheckCollision(entity->position, entity->size, entity->id))
		{
			entity->position = entityOldPosition;
		}
	}
	else
	{
		moving = false;
	}

	entity->boundingBox = Utilities_MakeBoundingBox(entity->position, entity->size);
	return moving;
}

void Entity_TakeDamage(Entity *entity, const int damageAmount)
{
	if (entity->type != SCENE_ACTOR)
	{
		return;
	}

	if (!entity->actor.dead)
	{
		entity->actor.health -= damageAmount;
		printf("entity id %d took %d damage\n", entity->id, damageAmount);
		if (entity->actor.health <= 0)
		{
			// Dirty hack to move bounding box outside of map so it cant be collided to.
			// We want to keep entity in the memory so we can use its position to display the
			// corpse/death anim
			const Vector3 deadBoxPos =
				(Vector3){ ACTOR_GRAVEYARD_POSITION, ACTOR_GRAVEYARD_POSITION, ACTOR_GRAVEYARD_POSITION };
			entity->boundingBox = Utilities_MakeBoundingBox(deadBoxPos, Vector3Zero());
			entity->actor.dead  = true;
		}
	}
}

bool Entity_FireAtPlayer(Entity *entity, float nextFire)
{

	if (entity->type != SCENE_ACTOR)
	{
		return false;
	}

	Entity_RotateTowards(entity, Player->position);
	if (nextFire > 0)
	{
		entity->actor.nextFire -= GetFrameTime();
		return false;
	}
	else
	{
		// Fire animation should play before we shoot projectile
		entity->actor.attacking = true;

		Projectile_Create(
			Entity_CreateRay(entity), (Vector3){ 0.2f, 0.2f, 0.2f }, entity->actor.damage, entity->id, PURPLE
		);
		entity->actor.nextFire = entity->actor.fireRate;
		return true;
	}
}

void Entity_RotateTowards(Entity *entity, const Vector3 targetPosition)
{
	if (entity->type == SCENE_ACTOR)
	{
		// Rotates the entity around Y axis
		const Vector3 diff        = Vector3Subtract(entity->position, targetPosition);
		const float y_angle       = -(atan2(diff.z, diff.x) + PI / 2.0);
		const Vector3 newRotation = (Vector3){ 0, y_angle, 0 };

		const Quaternion start = QuaternionFromEuler(entity->rotation.z, entity->rotation.y, entity->rotation.x);
		const Quaternion end   = QuaternionFromEuler(newRotation.z, newRotation.y, newRotation.x);
		const Quaternion slerp = QuaternionSlerp(start, end, entity->actor.rotationSpeed * GetFrameTime());

		entity->model.transform = QuaternionToMatrix(slerp);
		entity->rotation        = newRotation;
	}
}

// Creation functions
Entity Entity_CreateEnemy(const Vector3 position, const int id, const char *modelFileName)
{
	const Vector3 entityPosition = (Vector3){ position.x, ACTOR_POSITION_Y, position.z };
	const Vector3 entityRotation = Vector3Zero();
	const Vector3 entitySize     = (Vector3){ 0.25f, 1.1f, 0.25f };

	Entity enemy = { .type        = SCENE_ACTOR,
		             .boundingBox = Utilities_MakeBoundingBox(entityPosition, entitySize),
		             .id          = id,
		             .position    = entityPosition,
		             .rotation    = entityRotation,
		             .size        = entitySize,
		             .scale       = 0.5f,
		             .model       = LoadModel(modelFileName),
		             .actor       = Actor_Add(modelFileName) };
	return enemy;
}

Entity Entity_CreateWall(const char *textureFileName, const Vector3 position)
{

	Image textureImage = LoadImage(textureFileName);
	// The image has to be flipped since its loaded upside down
	ImageFlipVertical(&textureImage);
	const Texture2D texture = LoadTextureFromImage(textureImage);
	// Set map diffuse texture
	const Mesh cube                                           = GenMeshCube(1.0f, 1.0f, 1.0f);
	Model cubeModel                                           = LoadModelFromMesh(cube);
	cubeModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
	Vector3 size                                              = Vector3One();

	Entity wall = { .type        = SCENE_WALL,
		            .model       = cubeModel,
		            .position    = position,
		            .id          = WALL_MODEL_ID,
		            .size        = size,
		            .scale       = 1.0f,
		            .boundingBox = Utilities_MakeBoundingBox(position, size) };

	return wall;
}
