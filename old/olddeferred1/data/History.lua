local module={}
history=module

module.__index=module

function module.__newindex(t,k,b)
end

function module.new(size,v)
  local h={size=size,ind=1}

  for i=1,size do
    h[i]=v
  end

  setmetatable(h,module)
  return h
end

function module.ipairs(h)
  return h.iter, h, 0
end

function module.iter(h,i)
  i=i+1
  local ii=h.ind-i

  if ii<=0 then
    ii=h.size+ii
  end

  if i>h.size then
    return nil
  end

  return i,h[ii]
end

function module.ipairs2(h)
  return h.iter2, h, 0
end

function module.iter2(h,i)
  local ii=h.ind+i
  i=i+1

  if ii>h.size then
    ii=ii-h.size
  end

  if i>h.size then
    return nil
  end

  return i,h[ii]
end

function module.insert(h,v)
  h[h.ind]=v
  h.ind=h.ind+1

  if h.ind > h.size then
    h.ind=1
  end
end

function module.last(h)
  return h[h.ind]
end