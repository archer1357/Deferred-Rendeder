#include "ProgramManager.h"

bool ProgramManager::getSource(const std::string &fn,std::string &out) {
  std::map<std::string,std::string>::iterator it=sources.find(fn);

  if(it!=sources.end()) {
    out=it->second;
    return true;
  }

  std::string fn2=fn;
  std::ifstream file(fn2.c_str());

  if(!file) {
    std::cout << fn << ": could not open.\n";
    sources.insert(std::make_pair(fn,""));
    return false;
  }


  std::string source=std::string((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());

  sources.insert(std::make_pair(fn,source));
  out=source;
  return true;
}

bool ProgramManager::getShader(GLenum type, const std::string &fn,
                               const std::string &defines,GLuint &out) {

  std::string key;

  {
    std::stringstream ss;
    ss << fn << std::endl << type << std::endl << defines;
    key=ss.str();
  }

  std::map<std::string,GLuint>::iterator it=shaders.find(key);

  if(it!=shaders.end()) {
    out=it->second;
    return true;
  }

  std::string source;

  if(!getSource(fn,source)) {
    shaders.insert(std::make_pair(key,0));
    return false;
  }

  std::string head;

  if(verHigh>3 || (verHigh==3 && verLow>=30)) {
    head+="#version "+verStr+"\n";
  } else {
    head+="#version 130\n";
    head+="#extension GL_ARB_explicit_attrib_location : enable\n";
    head+="#extension GL_ARB_shading_language_420pack : enable\n";
    head+="#extension GL_ARB_uniform_buffer_object : enable\n";
    head+="#extension GL_ARB_shading_language_packing : enable\n";

    if(type==GL_GEOMETRY_SHADER) {
      std::cout << "thas\n";
      head+="#extension GL_ARB_geometry_shader4 : enable\n";
    }
  }

  head+=defines;
  head+="\n";
  head+="#line 0\n";

  //
  const char *source2[2]={head.c_str(),source.c_str()};

  GLuint shader=glCreateShader(type);
  glShaderSource(shader,2,source2,0);
  glCompileShader(shader);

  GLint status;
  glGetShaderiv(shader,GL_COMPILE_STATUS,&status);

  if(!status) {
    char log[256];
    glGetShaderInfoLog(shader,256,0,log);
    glDeleteShader(shader);
    std::cout << fn << ": shader compile error, " << log << std::endl;

    shaders.insert(std::make_pair(key,0));
    return false;
  }

  shaders.insert(std::make_pair(key,shader));
  out=shader;
  return true;
}

ProgramManager::ProgramManager() {
  std::string a= (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  size_t pos = a.find_last_of(".");
  std::string b=a.substr(0,pos);
  std::string c=a.substr(pos+1,std::string::npos);

  verHigh=atoi(b.c_str());
  verLow=atoi(c.c_str());

      std::stringstream ss;
    ss << verHigh << verLow;
    verStr=ss.str();

}

ProgramManager::~ProgramManager() {
  clear();
}
std::string dirnameOf(const std::string& fname) {
  size_t pos = fname.find_last_of("\\/");

  if(std::string::npos == pos) {
    return "";
  }

  return fname.substr(0, pos+1);
}
GLuint ProgramManager::get(const std::string &fn) {
  std::map<std::string,GLuint>::iterator it=programs.find(fn);

  if(it!=programs.end()) {
    return it->second;
  }

  //
  std::string fn2=fn;
std::string path=dirnameOf(fn2);
  jsoncons::json doc;
  std::string vsFn,gsFn,fsFn,vsDefines,fsDefines,gsDefines;
  try {
    doc = jsoncons::json::parse_file(fn2);

    vsFn=doc.get("vertex","").as_string();
    gsFn=doc.get("geometry","").as_string();
    fsFn=doc.get("fragment","").as_string();
	  
	  if(!vsFn.empty()) {
		  vsFn=path+vsFn;
	  }

	  if(!gsFn.empty()) {
		  gsFn=path+gsFn;
	  }

	  if(!fsFn.empty()) {
		  fsFn=path+fsFn;
	  }

    if(doc.has_member("vertex_defines")) {
      jsoncons::json obj=doc["vertex_defines"];

      if(obj.is_object()) {
        for(auto i=obj.begin_members();i!=obj.end_members();++i) {
          if(i->value().is_string()) {
            vsDefines+="#define "+ i->name()+" "+i->value().as_string()+"\n";
          }
        }
      }
    }

    if(doc.has_member("geometry_defines")) {
      jsoncons::json obj=doc["geometry_defines"];

      if(obj.is_object()) {
        for(auto i=obj.begin_members();i!=obj.end_members();++i) {
          if(i->value().is_string()) {
            gsDefines+="#define "+ i->name()+" "+i->value().as_string()+"\n";
          }
        }
      }
    }

    if(doc.has_member("fragment_defines")) {
      jsoncons::json obj=doc["fragment_defines"];

      if(obj.is_object()) {
        for(auto i=obj.begin_members();i!=obj.end_members();++i) {
          if(i->value().is_string()) {
            fsDefines+="#define "+ i->name()+" "+i->value().as_string()+"\n";
          }
        }
      }
    }

    if(doc.has_member("defines")) {
      jsoncons::json obj=doc["defines"];

      if(obj.is_object()) {
        for(auto i=obj.begin_members();i!=obj.end_members();++i) {
          if(i->value().is_string()) {
            gsDefines+="#define "+ i->name()+" "+i->value().as_string()+"\n";
            vsDefines+="#define "+ i->name()+" "+i->value().as_string()+"\n";
            fsDefines+="#define "+ i->name()+" "+i->value().as_string()+"\n";
          }
        }
      }
    }
  } catch (const std::exception& e) {
    std::cout << fn << " " << e.what() << std::endl;
    return 0;
  }
  //
  GLuint program=glCreateProgram();

  if(!vsFn.empty()) {
    GLuint shader;

    if(!getShader(GL_VERTEX_SHADER,vsFn,vsDefines,shader)) {
      std::cout << "Problem with vs from " << fn << std::endl;
      programs.insert(std::make_pair(fn,0));
      return 0;
    }

    glAttachShader(program,shader);
  }


  if(!gsFn.empty()) {
    GLuint shader;

    if(!getShader(GL_GEOMETRY_SHADER,gsFn,gsDefines,shader)) {
      std::cout << "Problem with gs from " << fn << std::endl;
      programs.insert(std::make_pair(fn,0));
      return 0;
    }

    glAttachShader(program,shader);
  }


  if(!fsFn.empty()) {
    GLuint shader;

    if(!getShader(GL_FRAGMENT_SHADER,fsFn,fsDefines,shader)) {
      std::cout << "Problem with fs from " << fn << std::endl;
      programs.insert(std::make_pair(fn,0));
      return 0;
    }

    glAttachShader(program,shader);
  }


  //
  glLinkProgram(program);

  GLint status;
  glGetProgramiv(program,GL_LINK_STATUS,&status);

  if(!status) {
    char log[256];
    glGetProgramInfoLog(program,256,0,log);
    glDeleteProgram(program);
    std::cout << fn << ": program link error, " << log << std::endl;
  }

  //

  programs.insert(std::make_pair(fn,program));
  return program;
}

void ProgramManager::onFileAdded(const std::string &fn) {
}
void ProgramManager::onFileModified(const std::string &fn) {
  //todo
  if(fn.length() >2) {
    std::string x=fn.substr(fn.length()-2,2);

    if(x=="vs" || x=="fs" || x=="gs") {
      clear();
    }
  } else if(fn.length() >9) {
    std::string x=fn.substr(fn.length()-9,9);
    
   // if(x==".json") {
   //   clear();
  //  }
  }
}
void ProgramManager::onFileDeleted(const std::string &fn) {
}
void ProgramManager::clear() {
  for(std::map<std::string,GLuint>::iterator i=programs.begin();i!=programs.end();i++) {
    glDeleteProgram(i->second);
  }

  for(std::map<std::string,GLuint>::iterator i=shaders.begin();i!=shaders.end();i++) {
    glDeleteShader(i->second);
  }

  programs.clear();
  shaders.clear();
  sources.clear();
}
