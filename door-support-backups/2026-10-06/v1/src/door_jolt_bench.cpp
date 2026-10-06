// Original isolated door fixture. Jolt bootstrap follows its CC0 HelloWorld.
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include "generated_fixture.hpp"
using namespace JPH;

struct BroadLayers final:BroadPhaseLayerInterface {
 uint GetNumBroadPhaseLayers()const override{return 2;}
 BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer l)const override{return BroadPhaseLayer(l);}
};
struct PairFilter final:ObjectLayerPairFilter {
 bool ShouldCollide(ObjectLayer a,ObjectLayer b)const override{return a==1||b==1;}
};
struct BroadFilter final:ObjectVsBroadPhaseLayerFilter {
 bool ShouldCollide(ObjectLayer a,BroadPhaseLayer b)const override{return a==1||b==BroadPhaseLayer(1);}
};
struct FloorContacts final:ContactListener {
 std::atomic<unsigned> count{0};
 void OnContactAdded(const Body&a,const Body&b,const ContactManifold&,ContactSettings&)override {
  if((a.GetUserData()==1&&b.GetUserData()==3)||(a.GetUserData()==3&&b.GetUserData()==1))++count;
 }
};
static void require(bool yes,const std::string& message){if(!yes){std::cerr<<message<<'\n';std::exit(2);}}
static Vec3 vec(P3 p){return Vec3(float(p.x),float(p.y),float(p.z));}
static RefConst<Shape> shape_result(ShapeSettings::ShapeResult r){require(!r.HasError(),r.HasError()?r.GetError().c_str():"shape error");return r.Get();}
static RefConst<Shape> frame_shape(const StageInput&s) {
 StaticCompoundShapeSettings compound;
 for(const auto&b:s.boxes){Vec3 lo=vec(b.minimum),hi=vec(b.maximum),half=(hi-lo)*.5f;float rad=std::min(.001f,.25f*std::min({half.GetX(),half.GetY(),half.GetZ()}));
  RefConst<Shape> box=shape_result(BoxShapeSettings(half,rad).Create());compound.AddShape((hi+lo)*.5f,Quat::sIdentity(),box);}
 return shape_result(compound.Create());
}
static RefConst<Shape> leaf_shape(const FixtureInput&f) {
 StaticCompoundShapeSettings compound;
 for(const auto&h:f.hulls){Array<Vec3>points;for(auto p:h.vertices)points.push_back(vec(p));ConvexHullShapeSettings hull(points,.001f);hull.mDensity=650;
  RefConst<Shape> shape=shape_result(hull.Create());compound.AddShape(Vec3::sZero(),Quat::sIdentity(),shape);}
 return shape_result(compound.Create());
}
#ifdef JPH_ENABLE_ASSERTS
static bool fail_assert(const char*expression,const char*message,const char*file,uint line){std::cerr<<file<<':'<<line<<' '<<expression<<' '<<(message?message:"")<<'\n';std::exit(3);}
#endif
int main(int argc,char**argv) {
 require(argc==2,"Pass an existing output directory");std::string out=argv[1];const FixtureInput fixture=make_fixture();
 require(fixture.stages.size()==3,"Expected 3 support stages");
 require(fixture.stages[0].support_count==3&&fixture.stages[1].support_count==1&&fixture.stages[2].support_count==0,"Contract must provide 3 -> 1 -> 0");
 RegisterDefaultAllocator();JPH_IF_ENABLE_ASSERTS(AssertFailed=fail_assert;)
 Factory::sInstance=new Factory();RegisterTypes();
 TempAllocatorImpl temp(8*1024*1024);JobSystemSingleThreaded jobs(cMaxPhysicsJobs);
 BroadLayers broad;PairFilter pair;BroadFilter broad_filter;FloorContacts contacts;
 PhysicsSystem physics;physics.Init(64,0,256,512,broad,broad_filter,pair);physics.SetGravity(Vec3(0,0,-9.81f));physics.SetContactListener(&contacts);
 BodyInterface&bi=physics.GetBodyInterface();
 BodyCreationSettings floor_settings(new BoxShape(Vec3(6,6,.1f)),RVec3(0,0,-.1f),Quat::sIdentity(),EMotionType::Static,0);floor_settings.mUserData=1;
 BodyID floor_id=bi.CreateAndAddBody(floor_settings,EActivation::DontActivate);
 RefConst<Shape> current_frame_shape=frame_shape(fixture.stages[0]);
 BodyCreationSettings frame_settings(current_frame_shape,RVec3(0,0,fixture.lift),Quat::sIdentity(),EMotionType::Static,0);frame_settings.mUserData=2;
 BodyID frame_id=bi.CreateAndAddBody(frame_settings,EActivation::DontActivate);
 BodyCreationSettings leaf_settings(leaf_shape(fixture),RVec3(fixture.leaf_origin.x,fixture.leaf_origin.y,fixture.leaf_origin.z+fixture.lift),Quat::sRotation(Vec3::sAxisZ(),float(fixture.rotation_z)),EMotionType::Dynamic,1);
 leaf_settings.mUserData=3;leaf_settings.mLinearDamping=0;leaf_settings.mAngularDamping=.05f;leaf_settings.mMotionQuality=EMotionQuality::LinearCast;
 BodyID leaf_id=bi.CreateAndAddBody(leaf_settings,EActivation::Activate);
 HingeConstraintSettings hinge_settings;hinge_settings.mSpace=EConstraintSpace::WorldSpace;
 hinge_settings.mPoint1=hinge_settings.mPoint2=RVec3(fixture.hinge_point.x,fixture.hinge_point.y,fixture.hinge_point.z+fixture.lift);
 hinge_settings.mHingeAxis1=hinge_settings.mHingeAxis2=Vec3::sAxisZ();hinge_settings.mNormalAxis1=hinge_settings.mNormalAxis2=Vec3::sAxisX();
 hinge_settings.mLimitsMin=-105.0f*JPH_PI/180.0f;hinge_settings.mLimitsMax=0;
 Ref<TwoBodyConstraint> hinge=bi.CreateConstraint(&hinge_settings,frame_id,leaf_id);physics.AddConstraint(hinge);physics.OptimizeBroadPhase();
 std::ofstream csv(out+"/jolt_poses.csv");require(bool(csv),"Cannot open pose output");csv<<std::setprecision(10)<<"step,time,stage,supports,joints,x,y,z,qx,qy,qz,qw,vx,vy,vz,active\n";
 unsigned stage=0,removals=0,activations=0,support_wakes=0,shape_updates=0;double supported_drift=0,initial_z=bi.GetPosition(leaf_id).GetZ(),release_z=initial_z,drop_025=0;
 constexpr float dt=1.0f/120.0f;
 for(unsigned step=0;step<=600;++step){
  if(step==120||step==240){stage=step==120?1:2;const auto&s=fixture.stages[stage];
   RefConst<Shape> replacement=frame_shape(s);Vec3 delta=replacement->GetCenterOfMass()-current_frame_shape->GetCenterOfMass();
   bi.SetShape(frame_id,replacement,false,EActivation::DontActivate);if(hinge!=nullptr)hinge->NotifyShapeChanged(frame_id,delta);current_frame_shape=replacement;++shape_updates;
   if(step==120){bi.ActivateBody(leaf_id);++support_wakes;}
   for(const auto&command:s.commands){if(command=="remove_constraint"){require(hinge!=nullptr,"Double constraint removal");physics.RemoveConstraint(hinge);hinge=nullptr;++removals;}
    else if(command=="activate_body"){bi.ActivateBody(leaf_id);++activations;}else require(false,"Unsupported command");}
   require((hinge!=nullptr)==s.hinge_active,"Constraint activity disagrees with support contract");if(step==240)release_z=bi.GetPosition(leaf_id).GetZ();
  }
  RVec3 p=bi.GetPosition(leaf_id);Quat q=bi.GetRotation(leaf_id);Vec3 v=bi.GetLinearVelocity(leaf_id);
  if(stage<2)supported_drift=std::max(supported_drift,std::abs(double(p.GetZ())-initial_z));
  if(step==270)drop_025=release_z-p.GetZ();
  csv<<step<<','<<step*double(dt)<<','<<fixture.stages[stage].name<<','<<fixture.stages[stage].support_count<<','<<physics.GetConstraints().size()<<','<<p.GetX()<<','<<p.GetY()<<','<<p.GetZ()<<','<<q.GetX()<<','<<q.GetY()<<','<<q.GetZ()<<','<<q.GetW()<<','<<v.GetX()<<','<<v.GetY()<<','<<v.GetZ()<<','<<bi.IsActive(leaf_id)<<'\n';
  if(step<600)require(physics.Update(dt,1,&temp,&jobs)==EPhysicsUpdateError::None,"Jolt update error");
 }
 double final_z=bi.GetPosition(leaf_id).GetZ();
 std::ofstream report(out+"/jolt_report.json");require(bool(report),"Cannot open report output");
 report<<std::setprecision(12)<<"{\n\"engine\":\"Jolt "<<JPH_VERSION_MAJOR<<'.'<<JPH_VERSION_MINOR<<'.'<<JPH_VERSION_PATCH<<"\",\n\"physics_executed\":true,\n\"source_asset_hash\":\""<<fixture.hash<<"\",\n\"supports_sequence\":[3,1,0],\n\"max_supported_drift_m\":"<<supported_drift<<",\n\"constraint_removals\":"<<removals<<",\n\"activation_calls\":"<<activations<<",\n\"support_change_activation_calls\":"<<support_wakes<<",\n\"frame_shape_updates\":"<<shape_updates<<",\n\"initial_origin_z_m\":"<<initial_z<<",\n\"final_origin_z_m\":"<<final_z<<",\n\"total_drop_m\":"<<initial_z-final_z<<",\n\"free_fall_drop_0_25_s_m\":"<<drop_025<<",\n\"floor_contact_count\":"<<contacts.count.load()<<",\n\"position_writes_after_creation\":0,\n\"fixture_lift_m\":"<<fixture.lift<<",\n\"time_step_s\":"<<dt<<",\n\"collision_proxy\":\"Approved wood convex hulls; frame AABBs with accepted CSG slot subtraction; decorative metal omitted\"\n}\n";
 require(hinge==nullptr,"Hinge survived zero supports");
 for(BodyID id:{leaf_id,frame_id,floor_id}){bi.RemoveBody(id);bi.DestroyBody(id);}
 UnregisterTypes();delete Factory::sInstance;Factory::sInstance=nullptr;
 std::cout<<"Jolt door fixture complete: hold drift="<<supported_drift<<" m, fall="<<initial_z-final_z<<" m, floor contacts="<<contacts.count.load()<<'\n';
 return 0;
}
