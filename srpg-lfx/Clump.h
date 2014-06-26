#import <Foundation/Foundation.h>
#import <SpriteKit/SpriteKit.h>
#import "mg.h"

/////// SUPPORT ///////

// coordTransfer.h
#ifndef COORDPAIR
#define COORDPAIR

@interface coordPair : NSObject
@property (assign, atomic) int x,y;
@end
@implementation coordPair
-(id)initWithCoords:(int)y :(int)x {
	self.x = x;
	self.y = y;
	return self;
}

@end

#endif

#include "config.h"


// lockedEntity
@interface LockedEntity : NSObject
@property (strong, nonatomic) SKSpriteNode *sprite;
@property (assign, atomic) int y;
@property (assign, atomic) int x;
@property (strong, nonatomic) NSString *key;
@end

//map tile
@interface Maptile : NSObject
@property (strong, nonatomic) SKSpriteNode *sprite;
@property (strong, nonatomic) NSString *name;
@property (assign, atomic) int key;
@property (assign, atomic) int height;
@property (readonly) float slowdown;
@property (assign, atomic) BOOL emit;
@property (assign, atomic) int emitStrength;
@property (assign, atomic) BOOL walkable;
@property (assign, atomic) BOOL highlighted;

-(void)setHeightTint;
-(void)updateHighlight;
@end

// MyScene.h
@interface MyScene : SKScene
@property (nonatomic) CGSize contentSize;
@property(nonatomic) CGPoint contentOffset;

-(void)setContentScale:(CGFloat)scale;
@end

// Map.h
@interface Map : NSObject {
	int lightalpha[MAPHEIGHT][MAPWIDTH];
	Maptile *tileset[MAPHEIGHT][MAPWIDTH];
}
@property (strong, nonatomic) MyScene* parent;
@property (strong, nonatomic) SKSpriteNode *parentNode;

-(void)tellPlayerToMove:(int)y :(int)x;
-(BOOL)valid:(int)y :(int)x;
-(Maptile*)read:(int)y :(int)x;
-(BOOL)write:(int)y :(int) x tile:(Maptile*)t;
-(int)light_read:(int)y :(int)x;
-(BOOL)light_write:(int)y :(int)x value:(int)v;
-(void)registerTiles;
-(void)loadMap:(NSString*)name;
-(void)registerTilesRelativeTo:(SKNode*) node;
@end


// PathFinder.h
@interface PathNode : NSObject
@property (assign, atomic) int y,x;
@property (strong, nonatomic) PathNode* parent;
@property (strong, nonatomic) PathNode* child;
-(id)initWithLocation:(int)y :(int)x;
-(void)addNode:(PathNode*)node;
@end


@interface PathFinder : NSObject
@property (strong, nonatomic) Map* map;
@property (strong, atomic) PathNode* initialNode;
-(PathNode*)findPath:(int)startX :(int)startY :(int)endX :(int)endY;
-(void) backgroundProcess:(int)startX :(int)startY :(int)endX :(int)endY;
@end

// DetachedEntity.h
#import <Foundation/Foundation.h>
#import <SpriteKit/SpriteKit.h>

@interface DetachedEntity : NSObject
@property (assign, atomic) int x,y,health;
@property (strong, nonatomic) SKSpriteNode *sprite;
@property (strong, nonatomic) NSString *key;
@property (strong, nonatomic) NSString *name;
@property (strong, nonatomic) PathFinder *pf;
@property (strong, atomic) PathNode *dset;
-(id)initWithSpriteAndPosition:(NSString*)key :(int)y :(int)x;
-(void)setParent:(Map*) ptr;
-(void)acceptDirectionSet:(PathNode*)topNode;
-(void)navigate:(coordPair*)pair;
-(void)tick;
-(coordPair*)positionAsPair;
@end
#import <UIKit/UIKit.h>
#import <SpriteKit/SpriteKit.h>

@interface ViewController : UIViewController<UIScrollViewDelegate>



@end



/////// IMPLEMENTATIONS ///////


// detached entity

@implementation DetachedEntity {
	Map *parent;
	
	coordPair *target;
	
}
-(id)init {
	self.health = 100;
	self.pf = [[PathFinder alloc] init];
	self.sprite.zPosition = 100;
	self.pf.initialNode = self.dset;
	return self;
}
-(id)initWithSpriteAndPosition:(NSString *)key :(int)y :(int)x {
	self.pf = [[PathFinder alloc] init];
	self.sprite.zPosition = 100;
	self.pf.initialNode = self.dset;
	return self;
}
-(id)initWithPosition:(int)y :(int)x {
	self.y = y;
	self.x = x;
	self.health = 100;
	self.pf.initialNode = self.dset;
	self.sprite.zPosition = 100;
	self.sprite.position = CGPointMake(x * 32, y * 32);
	self.pf = [[PathFinder alloc] init];
	return self;
}

-(void)teleport:(int)y :(int)x {
//	[self.sprite removeFromParent];
	self.x = x; self.y = y;
	self.sprite.position = CGPointMake(x * 32, y * 32);
	[parent read:y :x].highlighted = NO;
	[[parent read:y :x] updateHighlight] ;
//	[self.sprite addChild:parent.parentNode];
}

-(void) acceptDirectionSet:(PathNode*)topNode {
	self.dset = topNode;
}

-(void)abort {
	self.dset = nil; // is this okay?
}

-(void)advance {
	if (self.dset.parent == nil) {
		NSLog(@"parent is nil");
		//[self abort];
		return;
	}
	[self teleport:self.dset.y :self.dset.x];
	self.dset = self.dset.parent;
}

-(void)setParent:(Map*) ptr {
	parent = ptr;
}

-(coordPair*)positionAsPair {
	return [[coordPair alloc] initWithCoords:self.y :self.x];
}

-(void)tick {
	[self advance];
}

-(void)navigate:(coordPair *)pair {
	target = pair;
//	[self.pf backgroundProcess:self.x :self.y :target.x :target.y];
	self.dset = [self.pf findPath:self.y :self.x :target.y :target.x];
	while (self.dset.child != nil) {
		self.dset = self.dset.child;
	}
	
}
-(void)navigateBG {
	[self acceptDirectionSet:[self.pf findPath:self.x :self.y :target.x :target.y]];
}
@end


// map.m
@implementation Map {
	SKTexture *tmap[MAXTILES];
	NSMutableArray *detachedEntities;
}
-(id)init {
	detachedEntities = [[NSMutableArray alloc] init];
	return self;
}

-(void)assignTilePositions {
	int scale;
	if ([[UIScreen mainScreen] respondsToSelector:@selector(scale)] == YES && [[UIScreen mainScreen] scale] == 2.00) {
		// RETINA DISPLAY
		scale = 32;
    } else {
		scale = 32;
	}
	for (int y = 0; y < MAPHEIGHT; y++) {
		for (int x = 0; x < MAPWIDTH; x++) {
			tileset[y][x].sprite.position = CGPointMake(x * scale, y * scale);
		}
	}
}

-(void)registerTiles {
	[self assignTilePositions];
	for (int y = 0; y < MAPHEIGHT; y++) {
		for (int x = 0; x < MAPWIDTH; x++) {
			[self.parent addChild:tileset[y][x].sprite];
		}
	}
}

-(void)registerTilesRelativeTo:(SKSpriteNode*) node {
	[self assignTilePositions];
	for (int y = 0; y < MAPHEIGHT; y++) {
		for (int x = 0; x < MAPWIDTH; x++) {
			[node addChild:tileset[y][x].sprite];
		}
	}
	self.parentNode = node;
}
// validator function
-(BOOL)valid:(int)y :(int)x {
	return ((y >= 0 && y < MAPHEIGHT) && (x >= 0 && x < MAPWIDTH));
}
// tile map get
-(Maptile*)read:(int)y :(int)x {
	if ([self valid:y :x]) {
		return tileset[y][x];
	}
	return nil;
}
-(BOOL)write:(int)y :(int) x tile:(Maptile*)t {
	if ([self valid:y :x]) {
		tileset[y][x] = t;
		return YES;
	}
	
	return NO;
}
// lightmap
-(int)light_read:(int)y :(int)x {
	if ([self valid:y :x]) return lightalpha[y][x]; else return -1;
}

-(BOOL)light_write:(int)y :(int)x value:(int)v {
	if ([self valid:y :x]) {
		lightalpha[y][x] = v;
		return YES;
	}
	return NO;
}

-(SKTexture*)getTileSprite:(NSString*)key {
	if (tmap[[key intValue]] == nil) {
		tmap[[key intValue]] = [SKTexture textureWithImageNamed:[NSString stringWithFormat:@"tile%@", key]];
	}
	return tmap[[key intValue]];
}

-(NSString*)heightRegisterToSpriteKey:(int)reg {
	switch (reg) {
		case 0:
			return @"096";
			break;
		case 1:
			return @"012";
			break;
		case 2:
			return @"004";
			break;
		case 3:
			return (floor(drand48() == 0) ? @"091" : @"001");
			break;
		case 4:
			return (floor(drand48() == 0) ? @"000" : @"066");
			break;
		case 5:
			return @"000";
			break;
		case 6:
			return @"066";
			break;
		case 7:
			return @"000";
			break;
		case 8:
			return @"066";
			break;
		case 9:
			return @"065";
		default:
			break;
	}
	
	return @"005";
}

-(NSString*)getShrubbery {
	switch (arc4random()%6) {
		case 0:
			return @"222";
			break;
		case 1:
			return @"223";
			break;
		case 2:
			return @"224";
			break;
		case 3:
			return @"225";
		case 4:
			return @"226";
		case 5:
			return @"227";
		default:
			break;
	}
	return @"222";
}

// initialize file
-(void)loadMap:(NSString*)name {
	NSLog(@"loading file %@", name);
	NSError *err = nil;
	NSString *contents = [NSString stringWithContentsOfFile:[NSString stringWithFormat:@"%@/%@", [[NSBundle mainBundle]bundlePath], name] encoding:NSASCIIStringEncoding error:&err];
	if (err != nil) {
		NSLog(@"Something screwed up somewhere.");
	}
	NSArray *array = [contents componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
	array = [array filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"SELF != ''"]];
	for (int y = 0; y < MAPHEIGHT; y++) {
		for (int x = 0; x < MAPWIDTH; x++) {
			NSString *cell; int tmp;
			cell = array[(y * (MAPWIDTH)) + x];
			
			NSCharacterSet *charactersToRemove = [[NSCharacterSet alphanumericCharacterSet] invertedSet];
			cell = [[cell componentsSeparatedByCharactersInSet:charactersToRemove] componentsJoinedByString:@""];
			tmp = [cell intValue];
			cell = [self heightRegisterToSpriteKey:[cell intValue]];
			
			tileset[y][x] = [[Maptile alloc] init];
			tileset[y][x].sprite = [[SKSpriteNode alloc] initWithTexture:[self getTileSprite:cell]];
			//		[tileset[y][x].sprite setTexture:[self getTileSprite:cell]];
			tileset[y][x].height = tmp;
			tileset[y][x].walkable = YES;
			//			if (tmp == 0 || YES) {tileset[y][x].walkable = NO;} else {tileset[y][x].walkable = YES;}
			[tileset[y][x] setHeightTint];
		}
		NSLog(@"added tile row [%d]", y);
	}
	
	NSLog(@"Post-processing...");
	for (int y = 0; y < MAPHEIGHT; y++) {
		for (int x = 0; x < MAPWIDTH; x++) {
			for (int c = 0; c < 3; c++) {
				if (tileset[y][x].height > 5 && arc4random()%16 == 0) {
					LockedEntity *le = [[LockedEntity alloc] init];
					le.x = arc4random()%32;
					le.y = arc4random()%32;
					le.key = [self getShrubbery];
					le.sprite = [[SKSpriteNode alloc] initWithTexture:[self getTileSprite:le.key]];
					[tileset[y][x].sprite addChild:le.sprite];
				}
			}
		}
	}
	NSLog(@"Finished post-processing.");
	
}

-(coordPair*)convertMaptoNode:(coordPair*)pair {
	if (![self valid:pair.y :pair.x]) {
		return nil;
	}
	int scale;
	if ([[UIScreen mainScreen] respondsToSelector:@selector(scale)] == YES && [[UIScreen mainScreen] scale] == 2.00) {
		// RETINA DISPLAY
		scale = 32;
    } else {
		scale = 32;
	}
	return [[coordPair alloc] initWithCoords:pair.y :pair.x];
}

-(void)addDetachedEntity:(DetachedEntity*)ent {
	ent.sprite = [[SKSpriteNode alloc] initWithTexture:[self getTileSprite:ent.key]];
	ent.sprite.position = CGPointMake(ent.y * 32, ent.x * 32);
	ent.pf.map = self;
	[self.parentNode addChild:ent.sprite];
	[detachedEntities addObject:ent];
}

-(void)tickDetachedEntities {
	for (DetachedEntity *ent in detachedEntities) {
		[ent tick];
	}
}


@end

//locketEntity.m
@implementation LockedEntity
@end

// pathFinder.m
@implementation PathNode
-(id)init {
	self.parent = nil;
	self.child = nil;
	return self;
}
-(id)initWithLocation:(int)y :(int)x {
	self.parent = nil;
	self.child = nil;
	self.y = y;
	self.x = x;
	return self;
}
-(void)addNode:(PathNode*)node {
	PathNode *tmp = self;
	while (tmp.child != nil) {
		tmp = tmp.child;
	}
	tmp.child = node;
	tmp.child.parent = tmp; // weird pointer logic!
}
@end
@interface STPair : NSObject;
@property (strong, nonatomic) Map *map;
@property (strong, nonatomic) PathFinder *pf;
@property (assign, atomic) int targ_x, targ_y, start_x, start_y;
@end
@implementation STPair


@end
@interface PathFindNode : NSObject {
@public
	int nodeX,nodeY;
	int cost;
	PathFindNode *parentNode;
}
+(id)node;
@end
@implementation PathFindNode
+(id)node
{
	return [[PathFindNode alloc] init];
}
@end

@implementation PathFinder {
	
}
-(void) backgroundProcess:(int)startX :(int)startY :(int)endX :(int)endY {
	STPair *pair = [[STPair alloc] init];
	pair.start_x = startX;
	pair.start_y = startY;
	pair.targ_x = endX;
	pair.targ_y = endY;
	pair.pf = self;
	//	PathNode* retNode = [[PathNode alloc] init];
//	NSMutableArray *nodeMap = [[NSMutableArray alloc] init];
	//	[self performSelectorInBackground:@selector(startBG:) withObject:pair];
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
		[self startBG:pair];
		dispatch_async(dispatch_get_main_queue(), ^(void){
			//THEN UPDATE UI HERE
		});
	});
	
	//	reurn retNode;
}

-(void)startBG:(STPair*)stpair {
	self.initialNode = [self findPath:stpair.start_y :stpair.start_x :stpair.targ_y :stpair.targ_x];
	
	
}

-(BOOL)spaceIsBlocked:(int)x :(int)y;
{
	//general-purpose method to return whether a space is blocked
	//NSLog(@"Reading from map at position [%d, %d]. Walkable: %d", y, x, [self.map read:y :x].walkable);
	return (![self.map read:x :y].walkable);
}

-(PathFindNode*)nodeInArray:(NSMutableArray*)a withX:(int)x Y:(int)y
{
	//Quickie method to find a given node in the array with a specific x,y value
	NSEnumerator *e = [a objectEnumerator];
	PathFindNode *n;
	
	while((n = [e nextObject]))
	{
		if((n->nodeX == x) && (n->nodeY == y))
		{
			return n;
		}
	}
	
	return nil;
}
-(PathFindNode*)lowestCostNodeInArray:(NSMutableArray*)a
{
	//Finds the node in a given array which has the lowest cost
	PathFindNode *n, *lowest;
	lowest = nil;
	NSEnumerator *e = [a objectEnumerator];
	
	while((n = [e nextObject]))
	{
		if(lowest == nil)
		{
			lowest = n;
		}
		else
		{
			if(n->cost < lowest->cost)
			{
				lowest = n;
			}
		}
	}
	return lowest;
}



-(PathNode*)findPath:(int)startX :(int)startY :(int)endX :(int)endY
{
	//find path function. takes a starting point and end point and performs the A-Star algorithm
	//to find a path, if possible. Once a path is found it can be traced by following the last
	//node's parent nodes back to the start
	NSLog(@"Starting pathfind from [%d, %d][%d] to [%d,%d][%d]", startX,startY,[self.map read:startX :startY].height, endX,endY,[self.map read:endX :endY].height);
	int x,y;
	int newX,newY;
	int currentX,currentY;
	NSMutableArray *openList, *closedList;
	
	if((startX == endX) && (startY == endY)) {
		NSLog(@"same point");
		return nil; //make sure we're not already there
	}
	
	
	openList = [[NSMutableArray alloc] init]; //array to hold open nodes
	
	//	BOOL animate = [shouldAnimateButton state];
	//	if(animate)
	//		pointerToOpenList = openList;
	
	closedList = [[NSMutableArray alloc] init]; //array to hold closed nodes
	
	PathFindNode *currentNode = nil;
	PathFindNode *aNode = nil;
	
	//create our initial 'starting node', where we begin our search
	PathFindNode *startNode = [PathFindNode node];
	startNode->nodeX = startX;
	startNode->nodeY = startY;
	startNode->parentNode = nil;
	startNode->cost = 0;
	//add it to the open list to be examined
	[openList addObject: startNode];
	
	while([openList count])
	{
		//while there are nodes to be examined...
		//		NSLog(@"running loop");
		//get the lowest cost node so far:
		currentNode = [self lowestCostNodeInArray: openList];
		
		if((currentNode->nodeX == endX) && (currentNode->nodeY == endY))
		{
			//if the lowest cost node is the end node, we've found a path
			
			//********** PATH FOUND ********************
			NSLog(@"Found path");
			
			//*****************************************//
			//NOTE: Code below is for the Demo app to trace/mark the path
			aNode = currentNode->parentNode;
			PathNode *retNode = [[PathNode alloc] init];
			while(aNode->parentNode != nil)
			{
				[self.map read:aNode->nodeX :aNode->nodeY].highlighted = YES;
				[[self.map read:aNode->nodeX :aNode->nodeY] updateHighlight];
				[retNode addNode:[[PathNode alloc] initWithLocation:aNode->nodeX :aNode->nodeY]];
				[self.initialNode addNode:[[PathNode alloc] initWithLocation:aNode->nodeX :aNode->nodeY]];
				aNode = aNode->parentNode;
				
			}
			retNode = retNode.child; // advance down the chain
			retNode.parent = nil; // remove the first element
			
			NSLog(@"Finished successfully");
			return retNode;
			//*****************************************//
		}
		else
		{
			//...otherwise, examine this node.
			//remove it from open list, add it to closed:
			[closedList addObject: currentNode];
			[openList removeObject: currentNode];
			
			//lets keep track of our coordinates:
			currentX = currentNode->nodeX;
			currentY = currentNode->nodeY;
			
			//check all the surroundinganodes/tiles:
			for(y=-1;y<=1;y++)
			{
				newY = currentY+y;
				for(x=-1;x<=1;x++)
				{
					newX = currentX+x;
					if(y || x) //avoid 0,0
					{
						//simple bounds check for the demo app's array
						if((newX>=0)&&(newY>=0)&&(newX<MAPHEIGHT)&&(newY<MAPWIDTH))
						{
							//if the node isn't in the open list...
							if(![self nodeInArray: openList withX: newX Y:newY])
							{
								//and its not in the closed list...
								if(![self nodeInArray: closedList withX: newX Y:newY])
								{
									//and the space isn't blocked
									if(![self spaceIsBlocked: newX :newY])
									{
										//then add it to our open list and figure out
										//the 'cost':
										//										NSLog(@"Adding object to open list.");
										aNode = [PathFindNode node];
										aNode->nodeX = newX;
										aNode->nodeY = newY;
										aNode->parentNode = currentNode;
										if (newY != currentY && newX != currentX) {
											aNode->cost = currentNode->cost + 14;
											
										} else {
											aNode->cost = currentNode->cost + 10;
										}
										//										[self.map read:aNode->nodeX :aNode->nodeY].highlighted = YES;
										//										[[self.map read:aNode->nodeX :aNode->nodeY] updateHighlight];
										//Compute your cost here. This demo app uses a simple manhattan
										//distance, added to the existing cost
										aNode->cost += (abs((newX) - endX) + abs((newY) - endY)) * 10;
										
										[openList addObject: aNode];
										
										//										if(animate) //demo animation stuff
										//											[self display];
									}
								}
							}
						}
					}
				}
			}
		}
	}
	//**** NO PATH FOUND *****
	NSLog(@"no path found.");
	return nil;
}
@end

//maptile.m
@implementation Maptile
-(BOOL)init_tile:(int)spritekey {
	if (spritekey > SPRITEKEY_MAX || spritekey <= 0) {
		return 0;
	}
	self.sprite = [SKSpriteNode spriteNodeWithImageNamed:[NSString stringWithFormat:@"%d", spritekey]];
	self.walkable = YES;
	return YES;
	
}

-(void)setHeightTint {
	[self.sprite setColor:[UIColor colorWithRed:0 green:0 blue:0 alpha:1]];
	self.sprite.colorBlendFactor = .5 * (1 - (float)self.height/10.0f);
}

-(void)updateHighlight {
	if (self.highlighted) {
		[self.sprite setColor:[UIColor colorWithRed:1 green:1 blue:0 alpha:1]];
	} else {
		[self setHeightTint];
	}
	
}

@end

//myScene.m
typedef NS_ENUM(NSInteger, IIMySceneZPosition)
{
    kIIMySceneZPositionScrolling = 0,
    kIIMySceneZPositionVerticalAndHorizontalScrolling,
    kIIMySceneZPositionStatic,
};

@interface MyScene ()
//kIIMySceneZPositionScrolling
@property (nonatomic, weak) SKSpriteNode *spriteToScroll;
@property (nonatomic, weak) SKSpriteNode *spriteForScrollingGeometry;

//kIIMySceneZPositionStatic
@property (nonatomic, weak) SKSpriteNode *spriteForStaticGeometry;

//kIIMySceneZPositionVerticalAndHorizontalScrolling
@property (nonatomic, weak) SKSpriteNode *spriteToHostHorizontalAndVerticalScrolling;
@property (nonatomic, weak) SKSpriteNode *spriteForHorizontalScrolling;
@property (nonatomic, weak) SKSpriteNode *spriteForVerticalScrolling;
@end

@implementation MyScene {
	float mnscale;
	int pathState;
	int ctouchoffsetx, ctouchoffsety;
	CGPoint pathStart, pathEnd;
	DetachedEntity *player;
	Map *map;
	PathFinder *pf;
	NSTimer *ticker;
}

-(id)initWithSize:(CGSize)size {
	if (self = [super initWithSize:size]) {
		
        [self setAnchorPoint:(CGPoint){0,1}];
        SKSpriteNode *spriteToScroll = [SKSpriteNode spriteNodeWithColor:[SKColor clearColor] size:size];
        [spriteToScroll setAnchorPoint:(CGPoint){0,1}];
        [spriteToScroll setZPosition:kIIMySceneZPositionScrolling];
        [self addChild:spriteToScroll];
		
        //Overlay sprite to make anchor point 0,0 (lower left, default for SK)
        SKSpriteNode *spriteForScrollingGeometry = [SKSpriteNode spriteNodeWithColor:[SKColor clearColor] size:size];
        [spriteForScrollingGeometry setAnchorPoint:(CGPoint){0,0}];
        [spriteForScrollingGeometry setPosition:(CGPoint){0, -size.height}];
        [spriteToScroll addChild:spriteForScrollingGeometry];
		
        SKSpriteNode *spriteForStaticGeometry = [SKSpriteNode spriteNodeWithColor:[SKColor clearColor] size:size];
        [spriteForStaticGeometry setAnchorPoint:(CGPoint){0,0}];
        [spriteForStaticGeometry setPosition:(CGPoint){0, size.height}];
        [spriteForStaticGeometry setZPosition:kIIMySceneZPositionStatic];
        [self addChild:spriteForStaticGeometry];
		
        SKSpriteNode *spriteToHostHorizontalAndVerticalScrolling = [SKSpriteNode spriteNodeWithColor:[SKColor clearColor] size:size];
        [spriteToHostHorizontalAndVerticalScrolling setAnchorPoint:(CGPoint){0,0}];
        [spriteToHostHorizontalAndVerticalScrolling setPosition:(CGPoint){0, -size.height}];
        [spriteToHostHorizontalAndVerticalScrolling setZPosition:kIIMySceneZPositionVerticalAndHorizontalScrolling];
        [self addChild:spriteToHostHorizontalAndVerticalScrolling];
		
        CGSize upAndDownSize = size;
        upAndDownSize.width = 30;
        SKSpriteNode *spriteForVerticalScrolling = [SKSpriteNode spriteNodeWithColor:[SKColor clearColor] size:upAndDownSize];
        [spriteForVerticalScrolling setAnchorPoint:(CGPoint){0,0}];
        [spriteForVerticalScrolling setPosition:(CGPoint){0,30}];
        [spriteToHostHorizontalAndVerticalScrolling addChild:spriteForVerticalScrolling];
		
        CGSize leftToRightSize = size;
        leftToRightSize.height = 30;
        SKSpriteNode *spriteForHorizontalScrolling = [SKSpriteNode spriteNodeWithColor:[SKColor clearColor] size:leftToRightSize];
        [spriteForHorizontalScrolling setAnchorPoint:(CGPoint){0,0}];
        [spriteForHorizontalScrolling setPosition:(CGPoint){10,0}];
        [spriteToHostHorizontalAndVerticalScrolling addChild:spriteForHorizontalScrolling];
		
        //Test sprites for constrained Scrolling
        CGFloat labelPosition = -500.0;
        CGFloat stepSize = 50.0;
        while (labelPosition < 2000.0)
        {
            NSString *labelText = [NSString stringWithFormat:@"%5.0f", labelPosition];
			
            SKLabelNode *scrollingLabel = [SKLabelNode labelNodeWithFontNamed:@"HelveticaNeue"];
            [scrollingLabel setText:labelText];
            [scrollingLabel setFontSize:14.0];
            [scrollingLabel setFontColor:[SKColor darkGrayColor]];
            [scrollingLabel setPosition:(CGPoint){.x = 0.0, .y = labelPosition}];
            [scrollingLabel setHorizontalAlignmentMode:SKLabelHorizontalAlignmentModeLeft];
            [spriteForHorizontalScrolling addChild:scrollingLabel];
			
            scrollingLabel = [SKLabelNode labelNodeWithFontNamed:@"HelveticaNeue"];
            [scrollingLabel setText:labelText];
            [scrollingLabel setFontSize:14.0];
            [scrollingLabel setFontColor:[SKColor darkGrayColor]];
            [scrollingLabel setPosition:(CGPoint){.x = labelPosition, .y = 0.0}];
            [scrollingLabel setHorizontalAlignmentMode:SKLabelHorizontalAlignmentModeLeft];
            [scrollingLabel setZPosition:kIIMySceneZPositionVerticalAndHorizontalScrolling];
            [spriteForVerticalScrolling addChild:scrollingLabel];
            labelPosition += stepSize;
        }
		
        //Test sprites for scrolling and zooming
        SKSpriteNode *greenTestSprite = [SKSpriteNode spriteNodeWithColor:[SKColor greenColor]
                                                                     size:(CGSize){.width = size.width,
                                                                         .height = size.height*.25}];
        [greenTestSprite setName:@"greenTestSprite"];
        [greenTestSprite setAnchorPoint:(CGPoint){0,0}];
        [spriteForScrollingGeometry addChild:greenTestSprite];
		
        SKSpriteNode *blueTestSprite = [SKSpriteNode spriteNodeWithColor:[SKColor blueColor]
                                                                    size:(CGSize){.width = size.width*.25,
                                                                        .height = size.height*.25}];
        [blueTestSprite setName:@"blueTestSprite"];
        [blueTestSprite setAnchorPoint:(CGPoint){0,0}];
        [blueTestSprite setPosition:(CGPoint){.x = size.width * .25, .y = size.height *.65}];
        [spriteForScrollingGeometry addChild:blueTestSprite];
		
        //Test sprites for stationary sprites
        SKLabelNode *stationaryLabel = [SKLabelNode labelNodeWithFontNamed:@"HelveticaNeue"];
        [stationaryLabel setText:@"I'm not gonna move, nope, nope."];
        [stationaryLabel setFontSize:14.0];
        [stationaryLabel setFontColor:[SKColor darkGrayColor]];
        [stationaryLabel setPosition:(CGPoint){.x = 60.0, .y = 60.0}];
        [stationaryLabel setHorizontalAlignmentMode:SKLabelHorizontalAlignmentModeLeft];
        [spriteForStaticGeometry addChild:stationaryLabel];
		
        //Set properties
        _contentSize = size;
        _spriteToScroll = spriteToScroll;
        _spriteForScrollingGeometry = spriteForScrollingGeometry;
        _spriteForStaticGeometry = spriteForStaticGeometry;
        _spriteToHostHorizontalAndVerticalScrolling = spriteToHostHorizontalAndVerticalScrolling;
        _spriteForVerticalScrolling = spriteForVerticalScrolling;
        _spriteForHorizontalScrolling = spriteForHorizontalScrolling;
        _contentOffset = (CGPoint){0,0};
		
		
		
        /* Setup your scene here */
        
        self.backgroundColor = [SKColor colorWithRed:0.15 green:0.15 blue:0.3 alpha:1.0];
        
		//        SKLabelNode *myLabel = [SKLabelNode labelNodeWithFontNamed:@"System"];
		//
		//        myLabel.text = @"Hello, World!";
		//        myLabel.fontSize = 30;
		//        myLabel.position = CGPointMake(CGRectGetMidX(self.frame),
		//                                       CGRectGetMidY(self.frame));
        
		pf = [[PathFinder alloc] init];
		//		mapnode = [[SKNode alloc] init];
		pathState = 0;
		//        [self addChild:myLabel];
		map = [[Map alloc] init];
		map.parent = self;
		mnscale = 1;
		const char* tmpc = [[NSString stringWithFormat:@"%@/map1.map", [[NSBundle mainBundle]bundlePath] ] UTF8String];
		buildMap(tmpc);
		
		[map loadMap:@"map1.map"];
		NSLog(@"loaded tiles");
		[map registerTilesRelativeTo:spriteForScrollingGeometry];
		
		//		mapnode.position = CGPointMake(250,250);
		NSLog(@"Dimensions: %f by %f", self.scene.frame.size.height, self.scene.frame.size.width);
		
		[map read:90 :190].highlighted = YES;
		[[map read:90 :190] updateHighlight];
		pf.map = map;
		player = [[DetachedEntity alloc] initWithPosition:10 :10];
		player.key = @"297";
		[map addDetachedEntity:player];
		NSLog(@"player position: [%f,%f]", player.sprite.position.y, player.sprite.position.x);
		[player navigate:[[coordPair alloc] initWithCoords:20 :20]];
		//[pf backgroundProcess:1 :1 :20 :10 :node];
		ticker = [NSTimer scheduledTimerWithTimeInterval:1.0f target:self selector:@selector(updateContent) userInfo:nil repeats:YES];
		[ticker fire];
		
	}
    return self;
}
-(void)willMoveFromView:(SKView *)view{
	
}

-(void)detectPinch:(UIPinchGestureRecognizer*)uip {
	//	NSLog(@"detected pinch");
	//CGPoint loc = [uip locationInView:self.view];
	//	mapnode.position = CGPointMake(loc.x - ctouchoffsetx, loc.y - ctouchoffsety);
	
	//	[mapnode setScale:uip.scale];
	
}

-(void)tellPlayerToMove:(int)y :(int)x {
	[player navigate:[[coordPair alloc] initWithCoords:y :x]];
}

-(void)didMoveToView:(SKView *)view{
	//	UIView *uiv;
	//	uiv = [[UIView alloc] initWithFrame:self.frame];
	//	[uiv setUserInteractionEnabled:YES];
	//	UIPinchGestureRecognizer *prec;
	//	prec = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(detectPinch:)];
	//	[uiv addGestureRecognizer:prec];
	//	[view addSubview:uiv];
}


-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    /* Called when a touch begins */
	
    for (UITouch *touch in touches) {
//        CGPoint location = [touch locationInNode:self];
        CGPoint locInMap = [touch locationInNode:self.spriteToHostHorizontalAndVerticalScrolling];
		
		//     SKSpriteNode *sprite = [SKSpriteNode spriteNodeWithImageNamed:@"Spaceship"];
        
        //sprite.position = location;
		//       ctouchoffsetx = location.x - mapnode.position.x;
		//	ctouchoffsety = location.y - mapnode.position.y;
		
		//		if (location.x > 250 && location.y > 200) {
		//			[mapnode removeAllChildren];
		//			const char* tmpc = [[NSString stringWithFormat:@"%@/map1.map", [[NSBundle mainBundle]bundlePath] ] UTF8String];
		//			buildMap(tmpc);
		//			[map loadMap:@"map1.map"];
		//			[map registerTilesRelativeTo:mapnode];
		//		}
		
	    NSLog(@"Touch coordinates: (%f,%f)", locInMap.x, locInMap.y);
		/// SKAction *action = [SKAction rotateByAngle:M_PI duration:1];
        
		// [sprite runAction:[SKAction repeatActionForever:action]];
		
		
    }
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch *touch in touches) {
		CGPoint loc = [touch locationInNode:self];
		//		mapnode.position = CGPointMake(loc.x - ctouchoffsetx, loc.y - ctouchoffsety);
	}
	
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	//	for (UITouch *touch in touches) {
	//		CGPoint loc = [touch locationInNode:self];
	
	
	//	}
}

-(void)update:(CFTimeInterval)currentTime {
    /* Called before each frame is rendered */
}
-(void)didChangeSize:(CGSize)oldSize
{
    CGSize size = [self size];
	
    CGPoint lowerLeft = (CGPoint){0, -size.height};
	
    [self.spriteForStaticGeometry setSize:size];
    [self.spriteForStaticGeometry setPosition:lowerLeft];
	
    [self.spriteToHostHorizontalAndVerticalScrolling setSize:size];
    [self.spriteToHostHorizontalAndVerticalScrolling setPosition:lowerLeft];
}

-(void)setContentSize:(CGSize)contentSize
{
    if (!CGSizeEqualToSize(contentSize, _contentSize))
    {
        _contentSize = contentSize;
        [self.spriteToScroll setSize:contentSize];
        [self.spriteForScrollingGeometry setSize:contentSize];
        [self.spriteForScrollingGeometry setPosition:(CGPoint){0, -contentSize.height}];
        [self updateConstrainedScrollerSize];
    }
}

-(void)setContentOffset:(CGPoint)contentOffset
{
    _contentOffset = contentOffset;
    contentOffset.x *= -1;
    [self.spriteToScroll setPosition:contentOffset];
	
    CGPoint scrollingLowerLeft = [self.spriteForScrollingGeometry convertPoint:(CGPoint){0,0} toNode:self.spriteToHostHorizontalAndVerticalScrolling];
	
    CGPoint horizontalScrollingPosition = [self.spriteForHorizontalScrolling position];
    horizontalScrollingPosition.y = scrollingLowerLeft.y;
    [self.spriteForHorizontalScrolling setPosition:horizontalScrollingPosition];
	
    CGPoint verticalScrollingPosition = [self.spriteForVerticalScrolling position];
    verticalScrollingPosition.x = scrollingLowerLeft.x;
    [self.spriteForVerticalScrolling setPosition:verticalScrollingPosition];
}

-(void)setContentScale:(CGFloat)scale;
{
    [self.spriteToScroll setScale:scale];
    [self updateConstrainedScrollerSize];
}

-(void)updateConstrainedScrollerSize
{
	
    CGSize contentSize = [self contentSize];
    CGSize verticalSpriteSize = [self.spriteForVerticalScrolling size];
    verticalSpriteSize.height = contentSize.height;
    [self.spriteForVerticalScrolling setSize:verticalSpriteSize];
	
    CGSize horizontalSpriteSize = [self.spriteForHorizontalScrolling size];
    horizontalSpriteSize.width = contentSize.width;
    [self.spriteForHorizontalScrolling setSize:horizontalSpriteSize];
	
    CGFloat xScale = [self.spriteToScroll xScale];
    CGFloat yScale = [self.spriteToScroll yScale];
    [self.spriteForVerticalScrolling setYScale:yScale];
    [self.spriteForVerticalScrolling setXScale:xScale];
    [self.spriteForHorizontalScrolling setXScale:xScale];
    [self.spriteForHorizontalScrolling setYScale:yScale];
    CGFloat xScaleReciprocal = 1.0/xScale;
    CGFloat yScaleReciprocal = 1/yScale;
    for (SKNode *node in [self.spriteForVerticalScrolling children])
    {
        [node setXScale:xScaleReciprocal];
        [node setYScale:yScaleReciprocal];
    }
    for (SKNode *node in [self.spriteForHorizontalScrolling children])
    {
        [node setXScale:xScaleReciprocal];
        [node setYScale:yScaleReciprocal];
    }
	
    [self setContentOffset:self.contentOffset];
}
-(void)updateContent {
	NSLog(@"Ticked detached entities ");
	[map tickDetachedEntities];
	[player tick];
}

@end

// ViewController.m
static NSString * kViewTransformChanged = @"view transform changed";

@interface ViewController ()

@property(nonatomic, weak)MyScene *scene;
@property(nonatomic, weak)UIView *clearContentView;

@end
@implementation ViewController

- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
	
    // Configure the view.
    SKView * skView = (SKView *)self.view;
    skView.showsFPS = YES;
    skView.showsNodeCount = YES;
	
    // Create and configure the scene.
    MyScene *scene = [MyScene sceneWithSize:skView.bounds.size];
    scene.scaleMode = SKSceneScaleModeFill;
	
    // Present the scene.
    [skView presentScene:scene];
    _scene = scene;
	
    CGSize contentSize = CGSizeMake(MAPWIDTH * 32, MAPHEIGHT * 32);
	//    contentSize.height *= 1.5;
	//    contentSize.width *= 1.5;
    [scene setContentSize:contentSize];
	
    UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:skView.bounds];
    [scrollView setContentSize:contentSize];
	
    scrollView.delegate = self;
    [scrollView setMinimumZoomScale:0.1];
    [scrollView setMaximumZoomScale:2.0];
    [scrollView setIndicatorStyle:UIScrollViewIndicatorStyleWhite];
    UIView *clearContentView = [[UIView alloc] initWithFrame:(CGRect){.origin = CGPointZero, .size = contentSize}];
    [clearContentView setBackgroundColor:[UIColor clearColor]];
    [scrollView addSubview:clearContentView];
	
	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapHandler:)];
	[tap setNumberOfTapsRequired:1.0];
	[tap setNumberOfTouchesRequired:1.0];
	[scrollView addGestureRecognizer:tap];
	
    _clearContentView = clearContentView;
	
    [clearContentView addObserver:self
                       forKeyPath:@"transform"
                          options:NSKeyValueObservingOptionNew
                          context:&kViewTransformChanged];
    [skView addSubview:scrollView];
}

-(void)tapHandler:(UITapGestureRecognizer *)recognizer{
	CGPoint point = [recognizer locationOfTouch:0 inView:self.view];
//	point = [self.scene convertPoint:point toNode:self.scene.spriteForScrollingGeometry];
	point = [self.scene convertPointFromView:point];
	point = [self.scene convertPoint:point toNode:self.scene.spriteForScrollingGeometry];
	point.x = (((point.x / 32) - (trunc(point.x / 32) > .5)) ? trunc(point.x / 32) + 1: trunc(point.x / 32));
	point.y = (((point.y / 32) - (trunc(point.y / 32) > .5)) ? trunc(point.y / 32) + 1: trunc(point.y / 32));
	[self.scene tellPlayerToMove:point.y :point.x];
}
- (BOOL)shouldAutorotate
{
    return YES;
}

- (NSUInteger)supportedInterfaceOrientations
{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        return UIInterfaceOrientationMaskAllButUpsideDown;
    } else {
        return UIInterfaceOrientationMaskAll;
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Release any cached data, images, etc that aren't in use.
}
-(void)adjustContent:(UIScrollView *)scrollView
{
    CGFloat zoomScale = [scrollView zoomScale];
    [self.scene setContentScale:zoomScale];
    CGPoint contentOffset = [scrollView contentOffset];
    [self.scene setContentOffset:contentOffset];
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    [self adjustContent:scrollView];
}

-(UIView *)viewForZoomingInScrollView:(UIScrollView *)scrollView
{
    return self.clearContentView;
}

-(void)scrollViewDidTransform:(UIScrollView *)scrollView
{
    [self adjustContent:scrollView];
}

- (void)scrollViewDidEndZooming:(UIScrollView *)scrollView withView:(UIView *)view atScale:(CGFloat)scale; // scale between minimum and maximum. called after any 'bounce' animations
{
    [self adjustContent:scrollView];
}
#pragma mark - KVO

-(void)observeValueForKeyPath:(NSString *)keyPath
                     ofObject:(id)object
                       change:(NSDictionary *)change
                      context:(void *)context
{
    if (context == &kViewTransformChanged)
    {
        [self scrollViewDidTransform:(id)[(UIView *)object superview]];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

-(void)dealloc
{
    @try {
        [self.clearContentView removeObserver:self forKeyPath:@"transform"];
    }
    @catch (NSException *exception) {    }
    @finally {    }
}

@end
