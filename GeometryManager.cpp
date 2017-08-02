#include "GeometryManager.h"
#include <iostream>
#include <sstream>
#include <fstream>

GeometryManager::DrawHandle::DrawHandle() : geometry(0),geometryDraw(0),invalid(false) {
}
GeometryManager::DrawHandle::DrawHandle(Geometry *geometry,Geometry::Draw *geometryDraw) :
  geometry(geometry),geometryDraw(geometryDraw),invalid(false) {
}
GeometryManager::DrawHandle::DrawHandle(const DrawHandle& other) :
  geometry(other.geometry),geometryDraw(other.geometryDraw),invalid(other.invalid) {
}
void GeometryManager::DrawHandle::draw() {
  if(geometry && geometryDraw && geometry->ok && geometryDraw->count != 0) {
    if(geometry->indices) {
      intptr_t first2=geometryDraw->first;
      glDrawElements(geometryDraw->mode,geometryDraw->count,
        geometry->indices->type,(void*)first2);
    } else {
      glDrawArrays(geometryDraw->mode,geometryDraw->first,geometryDraw->count);
    }

    invalid=false;
  } else if(!invalid) {
    std::cerr << "Invalid draw handle.\n";
    invalid = true;
  }
}

GeometryManager::GeometryManager() : maxAttribs(0) {
}

GeometryManager::~GeometryManager() {
}

void GeometryManager::clear() {//
  for(auto i : vaos) {
    GLuint vao = i.second;
    glDeleteVertexArrays(1,&vao);
  }

  vaos.clear();

  //
  for(auto i : layouts) {
    Layout *layout = i.second;
    delete layout;
  }

  layouts.clear();

  //
  for(auto i : geometries) {
    Geometry *g = i.second;
    deleteGeometry(g);
  }

  geometries.clear();

}

GLuint GeometryManager::getVao(const std::string &geometryFn,const std::string &layoutFn) {
  if(geometryFn.empty() || layoutFn.empty()) {
    return 0;
  }

  //
  std::string key = geometryFn + " & " + layoutFn;

  auto it = vaos.find(key);

  if(it != vaos.end()) {
    return it->second;
  }

  //
  GLuint vao;
  glGenVertexArrays(1,&vao);
  loadVao(vao,geometryFn,layoutFn);
  vaos.insert(std::make_pair(key,vao));
  return vao;
}

GeometryManager::DrawHandle GeometryManager::getDraw(const std::string &geometryFn,const std::string &drawName) {
  Geometry *geom = getGeometry(geometryFn);
  auto drawIt = geom->draws.find(drawName);

  if(drawIt == geom->draws.end()) {
    Geometry::Draw *draw=new Geometry::Draw();
    draw->mode=0;
    draw->first=0;
    draw->count=0;
    drawIt=geom->draws.insert(std::make_pair(drawName,draw)).first;
  }

  return DrawHandle(geom,drawIt->second);
}
/*
GLuint GeometryManager::getShdVao(const std::string &geometryFn){
    if(geometryFn.empty()) {
        return 0;
    }

    //
    std::string key = geometryFn + " & shd";

    auto it = vaos.find(key);

    if(it != vaos.end()) {
        return it->second;
    }

    //
    GLuint vao;
    glGenVertexArrays(1,&vao);
    loadShdVao(vao,geometryFn);
    vaos.insert(std::make_pair(key,vao));
    return vao;
}

DrawHandle GeometryManager::getShdDraw(const std::string &geometryFn){
  Geometry *geom = getGeometry(geometryFn);
  return DrawHandle(geom,&geom->shdDraw);
}

bool GeometryManager::loadShdVao(GLuint vao,const std::string &geometryFn){
  //
  Geometry *geometry = getGeometry(geometryFn);
 
  //
  if(!geometry->ok ) {
    return false;
  }
 
  //
  glBindVertexArray(vao);

  //disable existing attribs
  retrieveMaxAttribs();

  for(int i=0;i<maxAttribs;i++) {
    glDisableVertexAttribArray(i);
  }

  //vertex buffers
  

    glBindBuffer(GL_ARRAY_BUFFER,geometry->shdVertices->buffer);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
    glEnableVertexAttribArray(0);
    
  //index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,geometry->shdIndices->buffer);

  return true;
}*/

void GeometryManager::releaseGeometryBuffers(Geometry *geometry) {
  //vertices
  for(auto j : geometry->vertices) {
    Geometry::Vertices *v = j.second;
    glDeleteBuffers(1,&v->buffer);
    delete v;
  }

  geometry->vertices.clear();

  //indices
  if(geometry->indices) {
    glDeleteBuffers(1,&geometry->indices->buffer);
    delete geometry->indices;
    geometry->indices=0;
  }

}

void GeometryManager::deleteGeometry(Geometry *geometry) {
  //
  releaseGeometryBuffers(geometry);

  //draws
  for(auto d : geometry->draws) {
    delete d.second;
  }

  geometry->draws.clear();

  //
  delete geometry;
}

void GeometryManager::retrieveMaxAttribs() {
  if(maxAttribs==0) {
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS,&maxAttribs);
  }
}

GeometryManager::Layout *GeometryManager::getLayout(const std::string &fn) {
  auto it = layouts.find(fn);

  if(it != layouts.end()) {
    return it->second;
  }

  //
  Layout *layout = new Layout;
  layout->ok=false;
  loadLayout(layout,fn);
  layouts.insert(std::make_pair(fn,layout));
  return layout;
}

GeometryManager::Geometry *GeometryManager::getGeometry(const std::string &fn) {
  auto it = geometries.find(fn);

  if(it != geometries.end()) {
    return it->second;
  }

  //
  Geometry *geometry = new Geometry();
  geometry->ok=false;
  geometry->indices=0;
  loadGeometry(geometry,fn);

  geometries.insert(std::make_pair(fn,geometry));
  return geometry;
}
/*
void removeDupls(std::map<std::string,const float*> &result,const float *verts,unsigned int vertsNum) {
    for(unsigned int i=0;i<vertsNum;i++) {
        const float *vert=&verts[i*3];
        std::string key=std::string("")+vert[0] + " & " + vert[1] + " & " + vert[2];
        
        auto it=result.find(key);
        
        if(it==result.end()) {
            result.insert(std::make_pair(key,vert));
        }
    }
}*/


bool GeometryManager::loadGeometry(Geometry *geometry,const std::string &fn) {

  if(!geometry->modified.update(fn)) {
    return false;
  }

  releaseGeometryBuffers(geometry);

  geometry->ok = false;

  glBindVertexArray(0);

  std::ifstream ifs(fn.c_str(),std::ios::binary);

  if(!ifs) {
    std::cerr << fn << " : Geometry : open error.\n";
    return false;
  }

  //
  char head;

  while(true) {
    ifs.read(&head,1);

    //
    if(ifs.eof()) {
      break;
    }

    //
    if(head=='v') {
      unsigned int nameSize;
      ifs.read((char*)&nameSize,4);

      char *name=new char[nameSize+1];
      name[nameSize]='\0';
      ifs.read(name,nameSize);

      unsigned int type;
      ifs.read((char*)&type,4);

      char size;
      ifs.read(&size,1);

      unsigned int dataSize;
      ifs.read((char*)&dataSize,4);

      char *data=new char[dataSize];
      ifs.read(data,dataSize);

      //
      Geometry::Vertices *vertices = new Geometry::Vertices;
      geometry->vertices.insert(std::make_pair(name,vertices));

      vertices->type = type;
      vertices->size = (int)size;

      //
      glGenBuffers(1,&vertices->buffer);
      glBindBuffer(GL_ARRAY_BUFFER,vertices->buffer);
      glBufferData(GL_ARRAY_BUFFER,dataSize,data,GL_STATIC_DRAW);

      //
      delete[] name;
      delete[] data;
    } else if(head=='i') {
      unsigned int type;
      ifs.read((char*)&type,4);

      unsigned int dataSize;
      ifs.read((char*)&dataSize,4);

      char *data=new char[dataSize];
      ifs.read(data,dataSize);

      //
      Geometry::Indices *indices = new Geometry::Indices();
      geometry->indices = indices;
      indices->type = type;

      glGenBuffers(1,&indices->buffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices->buffer);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER,dataSize,data,GL_STATIC_DRAW);

      //
      delete[] data;
    } else if(head=='d') {
      unsigned int nameSize;
      ifs.read((char*)&nameSize,4);

      char *name=new char[nameSize+1];
      name[nameSize]='\0';
      ifs.read(name,nameSize);

      unsigned int mode;
      ifs.read((char*)&mode,4);

      unsigned int first;
      ifs.read((char*)&first,4);

      unsigned int count;
      ifs.read((char*)&count,4);

      //
      auto drawIt=geometry->draws.find(name);

      if(drawIt==geometry->draws.end()) {
        drawIt=geometry->draws.insert(std::make_pair(name,new Geometry::Draw())).first;
      }

      Geometry::Draw *draw=drawIt->second;

      draw->mode=mode;
      draw->first=first;
      draw->count=count;

      if(geometry->indices->type == GL_UNSIGNED_INT) {
        draw->first = sizeof(int)*first;
      } else if(geometry->indices->type == GL_UNSIGNED_SHORT) {
        draw->first = sizeof(short)*first;
      } else if(geometry->indices->type == GL_UNSIGNED_BYTE) {
        draw->first = sizeof(char)*first;
      } else {
        draw->first=0;
      }

      //
      delete[] name;
    } else {
      std::cerr << fn << " : Geometry : parse error.\n";
      return false;
    }
  }
  
  //
  /*
    geometry->shdVertices=new Geometry::Vertices;
    geometry->shdIndices = new Geometry::Indices();
    
    Vertices *posVerts=geometry->vertices["positions"];
    
    //geometry->shdDraw;
    
   std::map<std::string,const float*> shdVertsMap;
  removeDupls(vertsMap,const float *verts,unsigned int vertsNum)
  */
  
  //

  std::cerr << fn << " : Geometry : loaded.\n";
  geometry->ok = true;
  return true;
}

bool GeometryManager::loadLayout(Layout *layout,const std::string &fn) {
  
  if(!layout->modified.update(fn)) {
    return false;
  }

  //
  layout->ok = false;

  std::ifstream file(fn.c_str());

  if(!file) {
    std::cerr << fn << " : GeometryLayout : open error.\n";
    return false;
  }


  std::string line;
  int index;
  std::string name;

  while(file.good()) {
    std::getline(file, line);
    if(!line.size()) {
      continue;
    }
    std::stringstream ss(line);

    if(!(ss >> index)) {
      std::cerr << fn << " : GeometryLayout : invalid index.\n";
      return false;
    }

    if(!(ss >> name)) {
      std::cerr << fn << " : GeometryLayout : invalid name.\n";
      return false;
    }

    layout->attribs.push_back(std::make_pair(index,name));
  }

  //
  std::cerr << fn << " : GeometryLayout : loaded.\n";
  layout->ok = true;
  return true;
}

bool GeometryManager::loadVao(GLuint vao,const std::string &geometryFn,const std::string &layoutFn) {
  //
  Geometry *geometry = getGeometry(geometryFn);
  Layout *layout = getLayout(layoutFn);

  //
  geometry->layoutDeps.insert(layoutFn);
  layout->geometryDeps.insert(geometryFn);

  //
  if(!geometry->ok || !layout->ok) {
    return false;
  }

  //
  glBindVertexArray(vao);

  //disable existing attribs
  retrieveMaxAttribs();

  for(int i=0;i<maxAttribs;i++) {
    glDisableVertexAttribArray(i);
  }

  //vertex buffers
  for(auto layoutAttrib : layout->attribs) {
    int index = layoutAttrib.first;
    std::string name = layoutAttrib.second;
    auto vertIt = geometry->vertices.find(name);

    if(vertIt == geometry->vertices.end()) {
      continue;
    }

    Geometry::Vertices *vert = vertIt->second;

    glBindBuffer(GL_ARRAY_BUFFER,vert->buffer);
    glVertexAttribPointer(index,vert->size,vert->type,GL_FALSE,0,0);
    glEnableVertexAttribArray(index);
  }

  //index buffer
  if(geometry->indices) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,geometry->indices->buffer);
  }

  return true;
}

bool GeometryManager::reloadVao(const std::string &geometryFn,const std::string &layoutFn) {
  auto vaoIt = vaos.find(geometryFn + " & " + layoutFn);

  if(vaoIt == vaos.end()) {
    return false;
  }

  return loadVao(vaoIt->second,geometryFn,layoutFn);
}

bool GeometryManager::reloadLayout(const std::string &fn) {
  std::string layoutFn = fn;
  auto vIt = layouts.find(layoutFn);

  if(vIt == layouts.end()) {
    return false;
  }

  //

  //
  Layout *layout = vIt->second;
  layout->modified.reset();

  if(!loadLayout(layout,fn)) {
    return false;
  }

  //
  for(auto geometryFn : layout->geometryDeps) {
    reloadVao(geometryFn,layoutFn);
  }

  return true;
}

bool GeometryManager::reloadGeometry(const std::string &fn) {
  std::string geometryFn = fn;
  auto gIt = geometries.find(geometryFn);

  if(gIt == geometries.end()) {
    return false;
  }

  Geometry *geometry = gIt->second;

  geometry->modified.reset();

  if(!loadGeometry(geometry,geometryFn)) {
    return false;
  }

  for(auto layoutFn : geometry->layoutDeps) {
    reloadVao(geometryFn,layoutFn);
  }

  return false;
}

bool GeometryManager::removeVao(const std::string &geometryFn,const std::string &layoutFn) {
  std::string key = geometryFn + " & " + layoutFn;

  auto vaoIt = vaos.find(key);

  if(vaoIt == vaos.end()) {
    return false;
  }

  glDeleteVertexArrays(1,&vaoIt->second);
  vaos.erase(vaoIt);

  //
  Geometry *geometry = getGeometry(geometryFn);
  Layout *layout = getLayout(layoutFn);

  //remove deps
  geometry->layoutDeps.erase(layoutFn);
  layout->geometryDeps.erase(geometryFn);

  //delete when unused  by any other vaos
  if(geometry->layoutDeps.empty()) {
    deleteGeometry(geometry);
    geometries.erase(geometryFn);
  }

  if(layout->geometryDeps.empty()) {
    delete layout;
    layouts.erase(layoutFn);
  }

  return true;
}

void GeometryManager::refresh() {
  std::set<std::pair<std::string,std::string>> updatingVaos;

  for(auto it : layouts) {
    const std::string &fn=it.first;
    Layout *layout=it.second;

    if(loadLayout(it.second,it.first)) {
      for(auto geomDep : it.second->geometryDeps) {
        updatingVaos.insert(std::make_pair(geomDep,it.first));
      }
    }
  }

  for(auto it : geometries) {
    const std::string &fn=it.first;
    Geometry *geometry=it.second;
 
    if(loadGeometry(geometry,fn)) {
      for(auto layoutDep : it.second->layoutDeps) {
        updatingVaos.insert(std::make_pair(fn,layoutDep));
      }
    }
  }

  for(auto it : updatingVaos) {
    //std::cerr << it.first << " & " << it.second << " : vao updated"<< std::endl;
    if(reloadVao(it.first,it.second)) {
      //std::cerr << "ok\n";
    } else {
      //std::cerr << "not ok\n";
    }
  }
}
